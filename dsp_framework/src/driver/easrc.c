/*
 * Copyright 2020 NXP
 */

#include "easrc.h"
#include "easrc_firmware.h"
#include "debug.h"

#define swap(x, y) { x ^= y; y ^= x; x ^= y; }

/**
 * gcd - calculate and return the greatest common divisor of 2 unsigned longs
 * @a: first value
 * @b: second value
 */
unsigned long gcd(unsigned long a, unsigned long b)
{
	unsigned long r = a | b;

	if (!a || !b)
		return r;

	/* Isolate lsbit of r */
	r &= -r;

	while (!(b & r))
		b >>= 1;
	if (b == r)
		return r;

	for (;;) {
		while (!(a & r))
			a >>= 1;
		if (a == r)
			return r;
		if (a == b)
			return a;

		if (a < b)
			swap(a, b);
		a -= b;
		a >>= 1;
		if (a & r)
			a += b;
		a >>= 1;
	}
}

static inline uint32_t bits_taps_to_val(unsigned int t)
{
	switch (t) {
	case EASRC_RS_32_TAPS:
		return 32;
	case EASRC_RS_64_TAPS:
		return 64;
	case EASRC_RS_128_TAPS:
		return 128;
	}

	return 0;
}

/*****************************************************************************
 *  Scale filter coefficients (64 bits float)
 *  For input float32 normalized range (1.0,-1.0) -> output int[16,24,32]:
 *      scale it by multiplying filter coefficients by 2^31
 *  For input int[16, 24, 32] -> output float32
 *      scale it by multiplying filter coefficients by 2^-15, 2^-23, 2^-31
 *  input:
 *      easrc:  Structure pointer of fsl_easrc
 *      filterIn : Pointer to non-scaled input filter
 *      shift:  The multiply factor
 *  output:
 *      filterOut: scaled filter
 *****************************************************************************/
static int NormalizedFilterForFloat32InIntOut(struct fsl_easrc *easrc,
					      uint64_t *filterIn,
					      uint64_t *filterOut,
					      int shift)
{
	uint64_t coef = *filterIn;
	int64_t exp  = (coef & 0x7ff0000000000000ll) >> 52;
	uint64_t coefOut;

	/*
	 * If exponent is zero (value == 0), or 7ff (value == NaNs)
	 * dont touch the content
	 */
	if (((coef & 0x7ff0000000000000ll) == 0) ||
	    ((coef & 0x7ff0000000000000ll) == ((uint64_t)0x7ff << 52))) {
		*filterOut = coef;
	} else {
		if ((shift > 0 && (shift + exp) >= 2047) ||
		    (shift < 0 && (exp + shift) <= 0)) {
			LOG( "coef error\n");
			return -EINVAL;
		}

		/* coefficient * 2^shift ==>  coefficient_exp + shift */
		exp += shift;
		coefOut = (uint64_t)(coef & 0x800FFFFFFFFFFFFFll) +
					((uint64_t)exp << 52);
		*filterOut = coefOut;
	}

	return 0;
}

/* resets the pointer of the coeff memory pointers */
static int fsl_coeff_mem_ptr_reset(struct fsl_easrc *easrc,
				   unsigned int ctx_id, int mem_type)
{
	int ret = 0;
	u32 reg, mask, val;

	if (!easrc)
		return -ENODEV;

	switch (mem_type) {
	case EASRC_PF_COEFF_MEM:
		/* This resets the prefilter memory pointer addr */
		if (ctx_id >= EASRC_CTX_MAX_NUM) {
			LOG1( "Invalid context id[%d]\n", ctx_id);
			return -EINVAL;
		}

		reg = REG_EASRC_CCE1(ctx_id);
		mask = EASRC_CCE1_COEF_MEM_RST_MASK;
		val = EASRC_CCE1_COEF_MEM_RST;
		break;
	case EASRC_RS_COEFF_MEM:
		/* This resets the resampling memory pointer addr */
		reg = REG_EASRC_CRCC;
		mask = EASRC_CRCC_RS_CPR_MASK;
		val = EASRC_CRCC_RS_CPR;
		break;
	default:
		LOG( "Unknown memory type\n");
		return -EINVAL;
	}

	/*  To reset the write pointer back to zero, the register field
	 *  ASRC_CTX_CTRL_EXT1x[PF_COEFF_MEM_RST] can be toggled from
	 *  0x0 to 0x1 to 0x0.
	 */
	write32_bit(easrc->paddr + reg, mask, 0);
	write32_bit(easrc->paddr + reg, mask, val);
	write32_bit(easrc->paddr + reg, mask, 0);

	return ret;
}

static int write_pf_coeff_mem(struct fsl_easrc *easrc, int ctx_id,
			      u64 *arr, int n_taps, int shift)
{
	int ret = 0;
	int i;
	u32 *r;
	u32 r0, r1;
	u64 tmp;

	/* If STx_NUM_TAPS is set to 0x0 then return */
	if (!n_taps)
		return 0;

	if (!arr) {
		LOG( "NULL buffer\n");
		return -EINVAL;
	}

	/* When switching between stages, the address pointer
	 * should be reset back to 0x0 before performing a write
	 */
	ret = fsl_coeff_mem_ptr_reset(easrc, ctx_id, EASRC_PF_COEFF_MEM);
	if (ret)
		return ret;

	for (i = 0; i < (n_taps + 1) / 2; i++) {
		ret = NormalizedFilterForFloat32InIntOut(easrc,
						   &arr[i],
						   &tmp,
						   shift);
		if (ret)
			return ret;

		r = (uint32_t *)&tmp;

		r0 = r[0] & EASRC_32b_MASK;
		r1 = r[1] & EASRC_32b_MASK;

		write32(easrc->paddr + REG_EASRC_PCF(ctx_id), r0);

		write32(easrc->paddr + REG_EASRC_PCF(ctx_id), r1);
	}

	return 0;
}

static int fsl_easrc_prefilter_config(struct fsl_easrc *easrc,
				      unsigned int ctx_id)
{
	struct fsl_easrc_context *ctx;
	struct asrc_firmware_hdr *hdr;
	struct prefil_params *prefil, *selected_prefil = NULL;
	u32 inrate, outrate, offset = 0;
	int ret, i;
	ret = 0;

	/* to modify prefilter coeficients, the user must perform
	 * a write in ASRC_PRE_COEFF_FIFO[COEFF_DATA] while the
	 * RUN_EN for that context is set to 0
	 */
	if (!easrc)
		return -ENODEV;

	if (ctx_id >= EASRC_CTX_MAX_NUM) {
		LOG1( "Invalid context id[%d]\n", ctx_id);
		return -EINVAL;
	}

	ctx = easrc->ctx[ctx_id];

	ctx->in_filled_sample = bits_taps_to_val(easrc->rs_num_taps) / 2;
	ctx->out_missed_sample = ctx->in_filled_sample *
				 ctx->out_params.sample_rate /
				 ctx->in_params.sample_rate;

	ctx->st1_num_taps = 0;
	ctx->st2_num_taps = 0;

	write32(easrc->paddr + REG_EASRC_CCE1(ctx_id), 0);

	write32(easrc->paddr + REG_EASRC_CCE2(ctx_id), 0);

	/* prefilter is enabled only when doing downsampling.
	 * When out_rate >= in_rate, pf will be in bypass mode
	 */
	if (ctx->out_params.sample_rate >= ctx->in_params.sample_rate) {
		if (ctx->out_params.sample_rate == ctx->in_params.sample_rate) {
			write32_bit(easrc->paddr + REG_EASRC_CCE1(ctx_id),
				    EASRC_CCE1_RS_BYPASS_MASK,
				    EASRC_CCE1_RS_BYPASS);
		}

		if (ctx->in_params.sample_format == SNDRV_PCM_FORMAT_FLOAT_LE &&
		    ctx->out_params.sample_format != SNDRV_PCM_FORMAT_FLOAT_LE) {
			ctx->st1_num_taps = 1;
			ctx->st1_coeff    = &easrc->const_coeff;
			ctx->st1_num_exp  = 1;
			ctx->st2_num_taps = 0;
			ctx->st1_addexp = 31;
		} else if (ctx->in_params.sample_format != SNDRV_PCM_FORMAT_FLOAT_LE &&
			   ctx->out_params.sample_format == SNDRV_PCM_FORMAT_FLOAT_LE) {
			ctx->st1_num_taps = 1;
			ctx->st1_coeff    = &easrc->const_coeff;
			ctx->st1_num_exp  = 1;
			ctx->st2_num_taps = 0;
			ctx->st1_addexp -= ctx->in_params.fmt.addexp;
		} else {
			ctx->st1_num_taps = 1;
			ctx->st1_coeff    = &easrc->const_coeff;
			ctx->st1_num_exp  = 1;
			ctx->st2_num_taps = 0;
		}
	} else {
		inrate = ctx->in_params.norm_rate;
		outrate = ctx->out_params.norm_rate;

		hdr = easrc->firmware_hdr;
		prefil = easrc->prefil;

		for (i = 0; i < hdr->prefil_scen; i++) {
			if (inrate == prefil[i].insr && outrate == prefil[i].outsr) {
				selected_prefil = &prefil[i];
				LOG4( "Selected prefilter: %u insr, %u outsr, %u st1_taps, %u st2_taps\n",
					selected_prefil->insr,
					selected_prefil->outsr,
					selected_prefil->st1_taps,
					selected_prefil->st2_taps);
				break;
			}
		}

		if (!selected_prefil) {
			LOG4( "Conversion from in ratio %u(%u) to out ratio %u(%u) is not supported\n",
				ctx->in_params.sample_rate,
				inrate,
				ctx->out_params.sample_rate, outrate);
			return -EINVAL;
		}

		/* in prefilter coeff array, first st1_num_taps represent the
		 * stage1 prefilter coefficients followed by next st2_num_taps
		 * representing stage 2 coefficients
		 */
		ctx->st1_num_taps = selected_prefil->st1_taps;
		ctx->st1_coeff    = selected_prefil->coeff;
		ctx->st1_num_exp  = selected_prefil->st1_exp;

		offset = ((selected_prefil->st1_taps + 1) / 2) *
				sizeof(selected_prefil->coeff[0]);
		ctx->st2_num_taps = selected_prefil->st2_taps;
		ctx->st2_coeff    = (uint64_t *)((uint32_t)selected_prefil->coeff + offset);

		if (ctx->in_params.sample_format == SNDRV_PCM_FORMAT_FLOAT_LE &&
		    ctx->out_params.sample_format != SNDRV_PCM_FORMAT_FLOAT_LE) {
			/* only change stage2 coefficient for 2 stage case */
			if (ctx->st2_num_taps > 0)
				ctx->st2_addexp = 31;
			else
				ctx->st1_addexp = 31;
		} else if (ctx->in_params.sample_format != SNDRV_PCM_FORMAT_FLOAT_LE &&
			   ctx->out_params.sample_format == SNDRV_PCM_FORMAT_FLOAT_LE) {
			if (ctx->st2_num_taps > 0)
				ctx->st2_addexp -= ctx->in_params.fmt.addexp;
			else
				ctx->st1_addexp -= ctx->in_params.fmt.addexp;
		}
	}

	ctx->in_filled_sample += (ctx->st1_num_taps / 2) * ctx->st1_num_exp +
				  ctx->st2_num_taps / 2;
	ctx->out_missed_sample = ctx->in_filled_sample *
				 ctx->out_params.sample_rate /
				 ctx->in_params.sample_rate;

	if (ctx->in_filled_sample * ctx->out_params.sample_rate %
				ctx->in_params.sample_rate != 0)
		ctx->out_missed_sample += 1;
	/* To modify the value of a prefilter coefficient, the user must
	 * perform a write to the register ASRC_PRE_COEFF_FIFOn[COEFF_DATA]
	 * while the respective context RUN_EN bit is set to 0b0
	 */
	write32_bit(easrc->paddr + REG_EASRC_CC(ctx_id),
		    EASRC_CC_EN_MASK, 0);

	if (ctx->st1_num_taps > EASRC_MAX_PF_TAPS) {
		LOG2( "ST1 taps [%d] mus be lower than %d\n",
			ctx->st1_num_taps, EASRC_MAX_PF_TAPS);
		ret = -EINVAL;
		goto ctx_error;
	}

	/* Update ctx ST1_NUM_TAPS in Context Control Extended 2 register */
	write32_bit(easrc->paddr + REG_EASRC_CCE2(ctx_id),
		    EASRC_CCE2_ST1_TAPS_MASK,
		    EASRC_CCE2_ST1_TAPS(ctx->st1_num_taps - 1));

	/* Prefilter Coefficient Write Select to write in ST1 coeff */
	write32_bit(easrc->paddr + REG_EASRC_CCE1(ctx_id),
		    EASRC_CCE1_COEF_WS_MASK,
		    (EASRC_PF_ST1_COEFF_WR << EASRC_CCE1_COEF_WS_SHIFT));

	ret = write_pf_coeff_mem(easrc, ctx_id,
				 ctx->st1_coeff,
				 ctx->st1_num_taps,
				 ctx->st1_addexp);
	if (ret)
		goto ctx_error;

	if (ctx->st2_num_taps > 0) {
		if (ctx->st2_num_taps + ctx->st1_num_taps > EASRC_MAX_PF_TAPS) {
			LOG2( "ST2 taps [%d] mus be lower than %d\n",
				ctx->st2_num_taps, EASRC_MAX_PF_TAPS);
			ret = -EINVAL;
			goto ctx_error;
		}

		write32_bit(easrc->paddr + REG_EASRC_CCE1(ctx_id),
					 EASRC_CCE1_PF_TSEN_MASK,
					 EASRC_CCE1_PF_TSEN);
		/*
		 * Enable prefilter stage1 writeback floating point
		 * which is used for FLOAT_LE case
		 */
		write32_bit(easrc->paddr + REG_EASRC_CCE1(ctx_id),
					 EASRC_CCE1_PF_ST1_WBFP_MASK,
					 EASRC_CCE1_PF_ST1_WBFP);

		write32_bit(easrc->paddr + REG_EASRC_CCE1(ctx_id),
					 EASRC_CCE1_PF_EXP_MASK,
					 EASRC_CCE1_PF_EXP(ctx->st1_num_exp - 1));

		/* Update ctx ST2_NUM_TAPS in Context Control Extended 2 reg */
		write32_bit(easrc->paddr + REG_EASRC_CCE2(ctx_id),
					 EASRC_CCE2_ST2_TAPS_MASK,
					 EASRC_CCE2_ST2_TAPS(ctx->st2_num_taps - 1));

		/* Prefilter Coefficient Write Select to write in ST2 coeff */
		write32_bit(easrc->paddr + REG_EASRC_CCE1(ctx_id),
					 EASRC_CCE1_COEF_WS_MASK,
					 EASRC_PF_ST2_COEFF_WR << EASRC_CCE1_COEF_WS_SHIFT);

		ret = write_pf_coeff_mem(easrc, ctx_id,
					 ctx->st2_coeff,
					 ctx->st2_num_taps,
					 ctx->st2_addexp);
		if (ret)
			goto ctx_error;
	}

	return 0;

ctx_error:
	return ret;
}

/* input args: easrc->rs_num_taps */
static int fsl_easrc_resampler_config(struct fsl_easrc *easrc)
{
	struct asrc_firmware_hdr *hdr =  easrc->firmware_hdr;
	struct interp_params *interp = easrc->interp;
	struct interp_params *selected_interp = NULL;
	unsigned int num_coeff;
	unsigned int i;
	u64 *arr;
	u32 r0, r1;
	u32 *r;
	int ret;
	int tmp;

	if (!hdr) {
		LOG( "firmware not loaded!\n");
		return -ENODEV;
	}

	for (i = 0; i < hdr->interp_scen; i++) {
		if ((interp[i].num_taps - 1) ==
				bits_taps_to_val(easrc->rs_num_taps)) {
			arr = interp[i].coeff;
			selected_interp = &interp[i];
			LOG2( "Selected interp_filter: %u taps - %u phases\n",
				selected_interp->num_taps,
				selected_interp->num_phases);
			break;
		}
	}

	if (!selected_interp) {
		LOG( "failed to get interpreter configuration\n");
		return -EINVAL;
	}

	/*
	 *  RS_LOW - first half of center tap of the sinc function
	 *  RS_HIGH - second half of center tap of the sinc function
	 *  This is due to the fact the resampling function must be
	 *  symetrical - i.e. odd number of taps
	 */
	r = (uint32_t *)&selected_interp->center_tap;
	r0 = r[0] & EASRC_32b_MASK;
	r1 = r[1] & EASRC_32b_MASK;

	/* note: if write by marco there have wrong data to reg,
	 * not find reason */
	write32(easrc->paddr + REG_EASRC_RCTCL, r0);

	//write32(easrc->paddr + REG_EASRC_RCTCH, EASRC_RCTCH_RS_CH(r1));
	write32(easrc->paddr + REG_EASRC_RCTCH, r1);

	/* Write Number of Resampling Coefficient Taps
	 * 00b - 32-Tap Resampling Filter
	 * 01b - 64-Tap Resampling Filter
	 * 10b - 128-Tap Resampling Filter
	 * 11b - N/A
	 */
	write32_bit(easrc->paddr + REG_EASRC_CRCC,
		    EASRC_CRCC_RS_TAPS_MASK,
		    easrc->rs_num_taps);

	/* Reset prefilter coefficient pointer back to 0 */
	ret = fsl_coeff_mem_ptr_reset(easrc, 0, EASRC_RS_COEFF_MEM);
	if (ret)
		return ret;

	/* When the filter is programmed to run in:
	 * 32-tap mode, 16-taps, 128-phases 4-coefficients per phase
	 * 64-tap mode, 32-taps, 64-phases 4-coefficients per phase
	 * 128-tap mode, 64-taps, 32-phases 4-coefficients per phase
	 * This means the number of writes is constant no matter
	 * the mode we are using
	 */
	num_coeff = 16 * 128 * 4;

	for (i = 0; i < num_coeff; i++) {
		r = (uint32_t *)&arr[i];
		r0 = r[0] & EASRC_32b_MASK;
		r1 = r[1] & EASRC_32b_MASK;
		write32(easrc->paddr + REG_EASRC_CRCM, r0);

		write32(easrc->paddr + REG_EASRC_CRCM, r1);
	}

	return 0;
}

/* According raw_fmt fill easrc data fmt */
static int fsl_easrc_process_format(struct fsl_easrc_context *ctx,
			      struct fsl_easrc_data_fmt *fmt, int raw_fmt)
{
	struct fsl_easrc *easrc = ctx->easrc;
	int width;
	int ret;

	if (!fmt)
		return -EINVAL;

	/* context input floating point format
	 * 0 - integer format
	 * 1 - single precision fp format
	 */
	//fmt->floating_point = !snd_pcm_format_linear(raw_fmt);
	fmt->floating_point = 0;
	fmt->sample_pos = 0;
	fmt->iec958 = 0;

	width = 16;
	/* get the data width */
	//switch (snd_pcm_format_width(raw_fmt)) {
	switch (width) {
	case 16:
		fmt->width = EASRC_WIDTH_16_BIT;
		fmt->addexp = 15;
		break;
	case 20:
		fmt->width = EASRC_WIDTH_20_BIT;
		fmt->addexp = 19;
		break;
	case 24:
		fmt->width = EASRC_WIDTH_24_BIT;
		fmt->addexp = 23;
		break;
	case 32:
		fmt->width = EASRC_WIDTH_32_BIT;
		fmt->addexp = 31;
		break;
	default:
		return -EINVAL;
	}

	switch (raw_fmt) {
	case SNDRV_PCM_FORMAT_IEC958_SUBFRAME_LE:
		fmt->width = easrc->bps_iec958[ctx->index];
		fmt->iec958 = 1;
		fmt->floating_point = 0;
		if (fmt->width == EASRC_WIDTH_16_BIT) {
			fmt->sample_pos = 12;
			fmt->addexp = 15;
		} else if (fmt->width == EASRC_WIDTH_20_BIT) {
			fmt->sample_pos = 8;
			fmt->addexp = 19;
		} else if (fmt->width == EASRC_WIDTH_24_BIT) {
			fmt->sample_pos = 4;
			fmt->addexp = 23;
		}
		break;
	default:
		break;
	}

	/* data endianness
	 * 0 - little-endian
	 * 1 - big-endian
	 */
	/*
	ret = snd_pcm_format_big_endian(raw_fmt);
	if (ret < 0)
		return ret;
	*/
	fmt->endianness = 0;
	/* input data sign
	 * 0b - signed format
	 * 1b - unsigned format
	 */
	//fmt->unsign = snd_pcm_format_unsigned(raw_fmt) > 0 ? 1 : 0;
	fmt->unsign = 0;

	return 0;
}

int fsl_easrc_set_ctx_format(struct fsl_easrc_context *ctx,
			     int *in_raw_format,
			     int *out_raw_format)
{
	struct fsl_easrc *easrc = ctx->easrc;
	struct fsl_easrc_data_fmt *in_fmt = &ctx->in_params.fmt;
	struct fsl_easrc_data_fmt *out_fmt = &ctx->out_params.fmt;
	int ret = 0;

	/* get the bitfield values for input data format */
	if (in_raw_format && out_raw_format) {
		ret = fsl_easrc_process_format(ctx, in_fmt, *in_raw_format);
		if (ret)
			return ret;
	}

	write32_bit(easrc->paddr + REG_EASRC_CC(ctx->index),
		    EASRC_CC_BPS_MASK,
		    EASRC_CC_BPS(in_fmt->width));

	write32_bit(easrc->paddr + REG_EASRC_CC(ctx->index),
		    EASRC_CC_ENDIANNESS_MASK,
		    in_fmt->endianness << EASRC_CC_ENDIANNESS_SHIFT);

	write32_bit(easrc->paddr + REG_EASRC_CC(ctx->index),
		    EASRC_CC_FMT_MASK,
		    in_fmt->floating_point << EASRC_CC_FMT_SHIFT);

	write32_bit(easrc->paddr + REG_EASRC_CC(ctx->index),
		    EASRC_CC_INSIGN_MASK,
		    in_fmt->unsign << EASRC_CC_INSIGN_SHIFT);

	/* In Sample Position */
	write32_bit(easrc->paddr + REG_EASRC_CC(ctx->index),
		    EASRC_CC_SAMPLE_POS_MASK,
		    EASRC_CC_SAMPLE_POS(in_fmt->sample_pos));

	/* get the bitfield values for input data format */
	if (in_raw_format && out_raw_format) {
		ret = fsl_easrc_process_format(ctx, out_fmt, *out_raw_format);
		if (ret)
			return ret;
	}
	/* note: fixed signed = 0, maybe casue sound error
	 * set out_fmt unsign = 0 according to snd_pcm_format_unsigned */
	out_fmt->unsign = 0;

	write32_bit(easrc->paddr + REG_EASRC_COC(ctx->index),
		    EASRC_COC_BPS_MASK,
		    EASRC_COC_BPS(out_fmt->width));

	write32_bit(easrc->paddr + REG_EASRC_COC(ctx->index),
		    EASRC_COC_ENDIANNESS_MASK,
		    out_fmt->endianness << EASRC_COC_ENDIANNESS_SHIFT);

	write32_bit(easrc->paddr + REG_EASRC_COC(ctx->index),
		    EASRC_COC_FMT_MASK,
		    out_fmt->floating_point << EASRC_COC_FMT_SHIFT);

	write32_bit(easrc->paddr + REG_EASRC_COC(ctx->index),
		    EASRC_COC_OUTSIGN_MASK,
		    out_fmt->unsign << EASRC_COC_OUTSIGN_SHIFT);

	/* Out Sample Position */
	write32_bit(easrc->paddr + REG_EASRC_COC(ctx->index),
		    EASRC_COC_SAMPLE_POS_MASK,
		    EASRC_COC_SAMPLE_POS(out_fmt->sample_pos));

	write32_bit(easrc->paddr + REG_EASRC_COC(ctx->index),
		    EASRC_COC_IEC_EN_MASK,
		    out_fmt->iec958 << EASRC_COC_IEC_EN_SHIFT);
	return ret;
}

/* set_rs_ratio
 *
 * According to the resample taps, calculate the resample ratio
 */
static int set_rs_ratio(struct fsl_easrc_context *ctx)
{
	struct fsl_easrc *easrc = ctx->easrc;
	unsigned int in_rate = ctx->in_params.norm_rate;
	unsigned int out_rate = ctx->out_params.norm_rate;
	unsigned int int_bits;
	unsigned int frac_bits;
	u64 val;
	u32 *r;

	switch (easrc->rs_num_taps) {
	case EASRC_RS_32_TAPS:
		int_bits = 5;
		frac_bits = 39;
		break;
	case EASRC_RS_64_TAPS:
		int_bits = 6;
		frac_bits = 38;
		break;
	case EASRC_RS_128_TAPS:
		int_bits = 7;
		frac_bits = 37;
		break;
	default:
		return -EINVAL;
	}

	val = ((uint64_t)in_rate << frac_bits) / out_rate;
	r = (uint32_t *)&val;

	if (r[1] & 0xFFFFF000) {
		LOG( "ratio exceed range\n");
		return -EINVAL;
	}

	write32(easrc->paddr + REG_EASRC_RRL(ctx->index),r[0]);
	write32(easrc->paddr + REG_EASRC_RRH(ctx->index),r[1]);

	return 0;
}

/* normalize input and output sample rates */
static void fsl_easrc_normalize_rates(struct fsl_easrc_context *ctx)
{
	int a, b;

	if (!ctx)
		return;

	a = ctx->in_params.sample_rate;
	b = ctx->out_params.sample_rate;

	a = gcd(a, b);

	/* divide by gcd to normalize the rate */
	ctx->in_params.norm_rate = ctx->in_params.sample_rate / a;
	ctx->out_params.norm_rate = ctx->out_params.sample_rate / a;
}

static int fsl_easrc_max_ch_for_slot(struct fsl_easrc_context *ctx,
				     struct fsl_easrc_slot *slot)
{
	int st1_mem_alloc = 0, st2_mem_alloc = 0;
	int pf_mem_alloc = 0;
	int max_channels = 8 - slot->num_channel;
	int channels = 0;

	if (ctx->st1_num_taps > 0) {
		if (ctx->st2_num_taps > 0)
			st1_mem_alloc =
				(ctx->st1_num_taps - 1) * ctx->st1_num_exp + 1;
		else
			st1_mem_alloc = ctx->st1_num_taps;
	}

	if (ctx->st2_num_taps > 0)
		st2_mem_alloc = ctx->st2_num_taps;

	pf_mem_alloc = st1_mem_alloc + st2_mem_alloc;

	if (pf_mem_alloc != 0)
		channels = (6144 - slot->pf_mem_used) / pf_mem_alloc;
	else
		channels = 8;

	if (channels < max_channels)
		max_channels = channels;

	return max_channels;
}

static int fsl_easrc_config_one_slot(struct fsl_easrc_context *ctx,
				     struct fsl_easrc_slot *slot,
				     unsigned int slot_idx,
				     unsigned int reg0,
				     unsigned int reg1,
				     unsigned int reg2,
				     unsigned int reg3,
				     unsigned int *req_channels,
				     unsigned int *start_channel,
				     unsigned int *avail_channel)
{
	struct fsl_easrc *easrc = ctx->easrc;
	int st1_chanxexp, st1_mem_alloc = 0, st2_mem_alloc = 0;
	unsigned int addr;
	int ret = 0;

	if (*req_channels <= *avail_channel) {
		slot->num_channel = *req_channels;
		slot->min_channel = *start_channel;
		slot->max_channel = *start_channel + *req_channels - 1;
		slot->ctx_index = ctx->index;
		slot->busy = true;
		*start_channel += *req_channels;
		*req_channels = 0;
	} else {
		slot->num_channel = *avail_channel;
		slot->min_channel = *start_channel;
		slot->max_channel = *start_channel + *avail_channel - 1;
		slot->ctx_index = ctx->index;
		slot->busy = true;
		*start_channel += *avail_channel;
		*req_channels -= *avail_channel;
	}

	write32_bit(easrc->paddr + reg0,
			  EASRC_DPCS0R0_MAXCH_MASK,
			  EASRC_DPCS0R0_MAXCH(slot->max_channel));

	write32_bit(easrc->paddr + reg0,
			  EASRC_DPCS0R0_MINCH_MASK,
			  EASRC_DPCS0R0_MINCH(slot->min_channel));

	write32_bit(easrc->paddr + reg0,
			  EASRC_DPCS0R0_NUMCH_MASK,
			  EASRC_DPCS0R0_NUMCH(slot->num_channel - 1));

	write32_bit(easrc->paddr +
			  reg0,
			  EASRC_DPCS0R0_CTXNUM_MASK,
			  EASRC_DPCS0R0_CTXNUM(slot->ctx_index));

	if (ctx->st1_num_taps > 0) {
		if (ctx->st2_num_taps > 0)
			st1_mem_alloc =
				(ctx->st1_num_taps - 1) * slot->num_channel *
				ctx->st1_num_exp + slot->num_channel;
		else
			st1_mem_alloc = ctx->st1_num_taps * slot->num_channel;

		slot->pf_mem_used = st1_mem_alloc;
		write32_bit(easrc->paddr + reg2,
				  EASRC_DPCS0R2_ST1_MA_MASK,
				  EASRC_DPCS0R2_ST1_MA(st1_mem_alloc));

		if (slot_idx == 1)
			addr = 0x1800 - st1_mem_alloc;
		else
			addr = 0;

		write32_bit(easrc->paddr + reg2,
				  EASRC_DPCS0R2_ST1_SA_MASK,
				  EASRC_DPCS0R2_ST1_SA(addr));
	}

	if (ctx->st2_num_taps > 0) {
		st1_chanxexp = slot->num_channel * (ctx->st1_num_exp - 1);

		write32_bit(easrc->paddr + reg1,
				  EASRC_DPCS0R1_ST1_EXP_MASK,
				  EASRC_DPCS0R1_ST1_EXP(st1_chanxexp));

		st2_mem_alloc = slot->num_channel * ctx->st2_num_taps;
		slot->pf_mem_used += st2_mem_alloc;
		write32_bit(easrc->paddr + reg3,
				  EASRC_DPCS0R3_ST2_MA_MASK,
				  EASRC_DPCS0R3_ST2_MA(st2_mem_alloc));

		if (slot_idx == 1)
			addr = 0x1800 - st1_mem_alloc - st2_mem_alloc;
		else
			addr = st1_mem_alloc;

		write32_bit(easrc->paddr + reg3,
				  EASRC_DPCS0R3_ST2_SA_MASK,
				  EASRC_DPCS0R3_ST2_SA(addr));
	}

	write32_bit(easrc->paddr + reg0,
			  EASRC_DPCS0R0_EN_MASK,
			  EASRC_DPCS0R0_EN);

	return ret;
}

/* fsl_easrc_config_slot
 *
 * A single context can be split amongst any of the 4 context processing pipes
 * in the design.
 * The total number of channels consumed within the context processor must be
 * less than or equal to 8. if a single context is configured to contain more
 * than 8 channels then it must be distributed across multiple context
 * processing pipe slots.
 *
 */
static int fsl_easrc_config_slot(struct fsl_easrc *easrc, unsigned int ctx_id)
{
	struct fsl_easrc_context *ctx = easrc->ctx[ctx_id];
	int req_channels = ctx->channels;
	int start_channel = 0, avail_channel;
	struct fsl_easrc_slot *slot0, *slot1;
	int i, ret;

	if (req_channels <= 0)
		return -EINVAL;

	for (i = 0; i < EASRC_CTX_MAX_NUM; i++) {
		slot0 = &easrc->slot[i][0];
		slot1 = &easrc->slot[i][1];

		if (slot0->busy && slot1->busy)
			continue;

		if (!slot0->busy) {
			if (slot1->busy && slot1->ctx_index == ctx->index)
				continue;

			avail_channel = fsl_easrc_max_ch_for_slot(ctx, slot1);
			if (avail_channel <= 0)
				continue;

			ret = fsl_easrc_config_one_slot(ctx,
							slot0, 0,
							REG_EASRC_DPCS0R0(i),
							REG_EASRC_DPCS0R1(i),
							REG_EASRC_DPCS0R2(i),
							REG_EASRC_DPCS0R3(i),
							&req_channels,
							&start_channel,
							&avail_channel);
			if (ret)
				return ret;

			if (req_channels > 0)
				continue;
			else
				break;
		}

		if (slot0->busy && !slot1->busy) {
			if (slot0->ctx_index == ctx->index)
				continue;

			avail_channel = fsl_easrc_max_ch_for_slot(ctx, slot0);
			if (avail_channel <= 0)
				continue;

			ret = fsl_easrc_config_one_slot(ctx,
							slot1, 1,
							REG_EASRC_DPCS1R0(i),
							REG_EASRC_DPCS1R1(i),
							REG_EASRC_DPCS1R2(i),
							REG_EASRC_DPCS1R3(i),
							&req_channels,
							&start_channel,
							&avail_channel);
			if (ret)
				return ret;

			if (req_channels > 0)
				continue;
			else
				break;
		}
	}

	if (req_channels > 0) {
		LOG("no avail slot.\n");
		return -EINVAL;
	}

	return 0;
}

/* fsl_easrc_config_context
 *
 * configure the register relate with context.
 */
int fsl_easrc_config_context(struct fsl_easrc *easrc, unsigned int ctx_id)
{
	struct fsl_easrc_context *ctx;
	int ret;

	/* to modify prefilter coeficients, the user must perform
	 * a write in ASRC_PRE_COEFF_FIFO[COEFF_DATA] while the
	 * RUN_EN for that context is set to 0
	 */
	if (!easrc)
		return -ENODEV;

	if (ctx_id >= EASRC_CTX_MAX_NUM) {
		LOG1( "Invalid context id[%d]\n", ctx_id);
		return -EINVAL;
	}

	ctx = easrc->ctx[ctx_id];

	fsl_easrc_normalize_rates(ctx);

	ret = set_rs_ratio(ctx);
	if (ret) {
		LOG1("set rs ratio failed, err: %d\n", ret);
		return ret;
	}

	/* initialize the context coeficients */
	ret = fsl_easrc_prefilter_config(easrc, ctx->index);
	if (ret) {
		LOG1("prefil config failed, err: %d\n", ret);
		return ret;
	}

	ret = fsl_easrc_config_slot(easrc, ctx->index);
	if (ret) {
		LOG1("config slot failed, err: %d\n", ret);
		return ret;
	}

	/* Both prefilter and resampling filters can use following
	 * initialization modes:
	 * 2 - zero-fil mode
	 * 1 - replication mode
	 * 0 - software control
	 */
	write32_bit(easrc->paddr + REG_EASRC_CCE1(ctx_id),
			  EASRC_CCE1_RS_INIT_MASK,
			  EASRC_CCE1_RS_INIT(ctx->rs_init_mode));

	write32_bit(easrc->paddr + REG_EASRC_CCE1(ctx_id),
			  EASRC_CCE1_PF_INIT_MASK,
			  EASRC_CCE1_PF_INIT(ctx->pf_init_mode));

	/* context input fifo watermark */
	write32_bit(easrc->paddr + REG_EASRC_CC(ctx_id),
			  EASRC_CC_FIFO_WTMK_MASK,
			  EASRC_CC_FIFO_WTMK(ctx->in_params.fifo_wtmk));

	/* context output fifo watermark */
	write32_bit(easrc->paddr + REG_EASRC_COC(ctx_id),
			  EASRC_COC_FIFO_WTMK_MASK,
			  EASRC_COC_FIFO_WTMK(ctx->out_params.fifo_wtmk - 1));

	/* number of channels */
	write32_bit(easrc->paddr + REG_EASRC_CC(ctx_id),
			  EASRC_CC_CHEN_MASK,
			  EASRC_CC_CHEN(ctx->channels - 1));
	return ret;
}

/* The ASRC provides interleaving support in hardware to ensure that a
 * variety of sample sources can be internally combined
 * to conform with this format. Interleaving parameters are accessed
 * through the ASRC_CTRL_IN_ACCESSa and ASRC_CTRL_OUT_ACCESSa registers
 */
int fsl_easrc_set_ctx_organziation(struct fsl_easrc_context *ctx)
{
	struct fsl_easrc *easrc;
	int ret = 0;

	if (!ctx)
		return -ENODEV;

	easrc = ctx->easrc;

	/* input interleaving parameters */
	write32_bit(easrc->paddr + REG_EASRC_CIA(ctx->index),
		    EASRC_CIA_ITER_MASK,
		    EASRC_CIA_ITER(ctx->in_params.iterations));

	write32_bit(easrc->paddr + REG_EASRC_CIA(ctx->index),
		    EASRC_CIA_GRLEN_MASK,
		    EASRC_CIA_GRLEN(ctx->in_params.group_len));

	write32_bit(easrc->paddr + REG_EASRC_CIA(ctx->index),
		    EASRC_CIA_ACCLEN_MASK,
		    EASRC_CIA_ACCLEN(ctx->in_params.access_len));

	/* output interleaving parameters */
	write32_bit(easrc->paddr + REG_EASRC_COA(ctx->index),
		    EASRC_COA_ITER_MASK,
		    EASRC_COA_ITER(ctx->out_params.iterations));

	write32_bit(easrc->paddr + REG_EASRC_COA(ctx->index),
		    EASRC_COA_GRLEN_MASK,
		    EASRC_COA_GRLEN(ctx->out_params.group_len));

	write32_bit(easrc->paddr + REG_EASRC_COA(ctx->index),
		    EASRC_COA_ACCLEN_MASK,
		    EASRC_COA_ACCLEN(ctx->out_params.access_len));

	return ret;
}

/* Request one of the available contexts
 *
 * Returns a negative number on error and >=0 as context id
 * on success
 */
int fsl_easrc_request_context(struct fsl_easrc_context *ctx,
			      unsigned int channels)
{
	enum asrc_pair_index index = ASRC_INVALID_PAIR;
	struct fsl_easrc *easrc = ctx->easrc;
	int ret = 0;
	int i;

	for (i = ASRC_PAIR_A; i < EASRC_CTX_MAX_NUM; i++) {
		if (easrc->ctx[i])
			continue;

		index = i;
		break;
	}

	if (index == ASRC_INVALID_PAIR) {
		LOG( "all contexts are busy\n");
		ret = -EBUSY;
	} else if (channels > easrc->chn_avail) {
		LOG1( "can't give the required channels: %d\n",
			channels);
		ret = -EINVAL;
	} else {
		ctx->index = index;
		ctx->channels = channels;
		easrc->ctx[index] = ctx;
		easrc->chn_avail -= channels;
	}

	return ret;
}

/* fixed easrc channel=1, in_format=2 */
int fsl_easrc_hw_params(volatile void *asrc_addr, int channels, int rate, int format, volatile void *context)
{
	struct fsl_easrc *easrc = (struct fsl_easrc *)asrc_addr;
	struct fsl_easrc_context *ctx = (struct fsl_easrc_context*)context;
	int stream;
	int ret;
	int playback = 0;

	/* connect ctx and easrc */
	ctx->easrc = easrc;

	ret = fsl_easrc_request_context(ctx, channels);
	if (ret) {
		LOG( "failed to request context\n");
		return ret;
	}

	/* set streams playback */
	stream = 0;
	//ctx->ctx_streams |= BIT(stream);
	ctx->ctx_streams = 0;

	/* set the input and output ratio so we can compute
	 * the resampling ratio in RS_LOW/HIGH
	 */
	if (stream == playback) {
		ctx->in_params.sample_rate = rate;
		ctx->in_params.sample_format = format;
		ctx->out_params.sample_rate = easrc->easrc_rate;
		ctx->out_params.sample_format = easrc->easrc_format;
	} else {
		ctx->out_params.sample_rate = rate;
		ctx->out_params.sample_format = format;
		ctx->in_params.sample_rate = easrc->easrc_rate;
		ctx->in_params.sample_format = easrc->easrc_format;
	}

	ctx->channels = channels;
	ctx->in_params.fifo_wtmk  = 0x20;
	ctx->out_params.fifo_wtmk = 0x20;

	/* do only rate conversion and keep the same format for input
	 * and output data
	 */
	ret = fsl_easrc_set_ctx_format(ctx,
				       &ctx->in_params.sample_format,
				       &ctx->out_params.sample_format);
	if (ret) {
		LOG1( "failed to set format %d", ret);
		return ret;
	}

	ret = fsl_easrc_config_context(easrc, ctx->index);
	if (ret) {
		LOG( "failed to config context\n");
		return ret;
	}

	ctx->in_params.iterations = 1;
	ctx->in_params.group_len = ctx->channels;
	ctx->in_params.access_len = ctx->channels;
	ctx->out_params.iterations = 1;
	ctx->out_params.group_len = ctx->channels;
	ctx->out_params.access_len = ctx->channels;

	ret = fsl_easrc_set_ctx_organziation(ctx);
	if (ret) {
		LOG( "failed to set fifo organization\n");
		return ret;
	}

	return 0;
}

void easrc_probe(struct fsl_easrc *easrc, int rstap, int easrc_rate, int width)
{
	/*Set default value*/
	easrc->chn_avail = 32;
	easrc->const_coeff = 0x3FF0000000000000ULL;

	switch (rstap) {
	case 32:
		easrc->rs_num_taps = EASRC_RS_32_TAPS;
		break;
	case 64:
		easrc->rs_num_taps = EASRC_RS_64_TAPS;
		break;
	case 128:
		easrc->rs_num_taps = EASRC_RS_128_TAPS;
		break;
	default:
		easrc->rs_num_taps = EASRC_RS_32_TAPS;
		break;
	}

	easrc->easrc_rate = easrc_rate;

	if (width != 16 && width != 24 && width != 32 && width != 20) {
		LOG("unsupported width, switching to 24bit\n");
		width = 24;
	}
	if (width == 24)
		easrc->easrc_format = 6;
	else if (width == 16)
		easrc->easrc_format = 2;
	else
		easrc->easrc_format = 10;

	return;
}

void easrc_init(volatile void * asrc_addr, int mode, int channels, int in_rate,
	       int width, int mclk_rate)
{
	struct fsl_easrc *easrc = (struct fsl_easrc *)asrc_addr;
	struct fsl_easrc_context *ctx;
	/* default registers */
	u32 tmp_cache[120] = {0};
	tmp_cache[80] = 0x7fffffff;
	tmp_cache[81] = 0x7fffffff;
	tmp_cache[82] = 0x7fffffff;
	tmp_cache[83] = 0x7fffffff;
	tmp_cache[94] = 0xfff;

	/* fixed outrate=48000, width=16 */
	easrc_probe(easrc, 0, 48000, 16);

	easrc_resume(easrc, tmp_cache);
	LOG("easrc_inited\n");
}

static int easrc_get_firmware(struct fsl_easrc *easrc)
{
	easrc->firmware_hdr = (struct asrc_firmware_hdr *)(&firm_hdr);

	easrc->interp = (struct interp_params *)(interp_arr);

	easrc->prefil = (struct prefil_params *)(prefil_arr);

	return 0;
}

/* Start the context
 *
 * Enable the DMA request and context
 */
int fsl_easrc_start_context(struct fsl_easrc_context *ctx)
{
	struct fsl_easrc *easrc = ctx->easrc;
	int ret;

	/* clear run stop */
	write32_bit(easrc->paddr + REG_EASRC_CC(ctx->index),
			EASRC_CC_STOP_MASK, 0);

	write32_bit(easrc->paddr + REG_EASRC_CC(ctx->index),
			  EASRC_CC_FWMDE_MASK,
			  EASRC_CC_FWMDE);

	write32_bit(easrc->paddr + REG_EASRC_COC(ctx->index),
			  EASRC_COC_FWMDE_MASK,
			  EASRC_COC_FWMDE);

	write32_bit(easrc->paddr + REG_EASRC_CC(ctx->index),
			  EASRC_CC_EN_MASK,
			  EASRC_CC_EN);

	return 0;
}

/* Stop the context
 *
 * Disable the DMA request and context
 */
int fsl_easrc_stop_context(struct fsl_easrc_context *ctx)
{
	struct fsl_easrc *easrc = ctx->easrc;
	int ret, val, i;
	int size = 0;
	int retry = 200;

	val = read32(easrc->paddr + REG_EASRC_CC(ctx->index));

	if (val & EASRC_CC_EN_MASK) {
		write32_bit(easrc->paddr + REG_EASRC_CC(ctx->index),
					 EASRC_CC_STOP_MASK, EASRC_CC_STOP);

		do {
			val = read32(easrc->paddr + REG_EASRC_SFS(ctx->index));
			val &= EASRC_SFS_NSGO_MASK;
			size = val >> EASRC_SFS_NSGO_SHIFT;

			/* Read FIFO, drop the data */
			for (i = 0; i < size * ctx->channels; i++)
				val = read32(easrc->paddr + REG_EASRC_RDFIFO(ctx->index));
			/* Check RUN_STOP_DONE */
			read32(easrc->paddr, REG_EASRC_IRQF);
			if (val & EASRC_IRQF_RSD(1 << ctx->index)) {
				/*Clear RUN_STOP_DONE*/
				write32_bit(easrc->paddr + REG_EASRC_IRQF,
					     EASRC_IRQF_RSD(1 << ctx->index),
					     EASRC_IRQF_RSD(1 << ctx->index));
				break;
			}
			//udelay(100);
		} while (--retry);

		if (retry == 0)
			LOG( "RUN STOP fail\n");
	}

	write32_bit(easrc->paddr + REG_EASRC_CC(ctx->index),
				 EASRC_CC_EN_MASK | EASRC_CC_STOP_MASK, 0);

	write32_bit(easrc->paddr + REG_EASRC_CC(ctx->index),
				 EASRC_CC_FWMDE_MASK, 0);

	write32_bit(easrc->paddr + REG_EASRC_COC(ctx->index),
				 EASRC_COC_FWMDE_MASK, 0);

	return 0;
}

void easrc_start(volatile void *asrc_addr, int tx)
{
	struct fsl_easrc *easrc = (struct fsl_easrc *)asrc_addr;
	struct fsl_easrc_context *ctx = NULL;
	int i;

	LOG("easrc start\n");
	for(i = 0; i < EASRC_CTX_MAX_NUM; i++) {
		if(!easrc->ctx[i])
			continue;
		ctx = easrc->ctx[i];
		break;
	}
	if (!ctx)
		return;
	fsl_easrc_start_context(ctx);
}
void easrc_stop(volatile void *asrc_addr, int tx)
{
	struct fsl_easrc *easrc = (struct fsl_easrc *)asrc_addr;
	struct fsl_easrc_context *ctx = NULL;
	int i;

	LOG("easrc stop\n");
	for(i = 0; i < EASRC_CTX_MAX_NUM; i++) {
		if(!easrc->ctx[i])
			continue;
		ctx = easrc->ctx[i];
		break;
	}
	if (!ctx)
		return;
	fsl_easrc_stop_context(ctx);
}
void easrc_irq_handler(volatile void *asrc_addr)
{
	struct fsl_easrc *easrc = (struct fsl_easrc *)asrc_addr;
	/* TODO treat the interrupt */
	return;
}

void register_save(struct fsl_easrc *easrc, u32 *cache_addr)
{
	/* escape in fifo and out fifo */
	int i;
	for (i = 8; i < 120; i++) {
		/* Resampling coef mem write only */
		if (i == 92 || (i >= 86 && i < 88))
			continue;
		cache_addr[i] = read32(easrc->paddr + i * 4);
	}
}

void easrc_suspend(volatile void *asrc_addr,  u32 *cache_addr)
{
	struct fsl_easrc *easrc = (struct fsl_easrc *)asrc_addr;
	register_save(easrc, cache_addr);
	easrc->firmware_loaded = 0;
}

void register_resume(struct fsl_easrc *easrc, u32 *cache_addr)
{
	int i;
	for (i = 8; i < 120; i++) {
		/* fifo status read only and escape
		 * resampling coef mem */
		if ((i >= 65 && i < 68) || i == 92 || i == 95)
			continue;
		write32(easrc->paddr + i * 4, cache_addr[i]);
	}
}

void easrc_resume(volatile void *asrc_addr,  u32 *cache_addr)
{
	struct fsl_easrc *easrc = (struct fsl_easrc *)asrc_addr;
	struct fsl_easrc_context *ctx;
	int ret, i;

	register_resume(easrc, cache_addr);

	if (easrc->firmware_loaded) {
		goto skip_load;
	}
	easrc->firmware_loaded = 1;

	ret = easrc_get_firmware(easrc);
	if (ret) {
		LOG1("failed to get firmware, err %d\n", ret);
		goto skip_load;
	}

	/* Write Resampling Coefficients
	 * The coefficient RAM must be configured prior to beginning of
	 * any context processing within the ASRC
	 */
	ret = fsl_easrc_resampler_config(easrc);
	if (ret) {
		LOG( "resampler config failed\n");
		goto skip_load;
	}

	for (i = ASRC_PAIR_A; i < EASRC_CTX_MAX_NUM; i++) {
		ctx = easrc->ctx[i];
		if (ctx) {
			set_rs_ratio(ctx);
			ctx->out_missed_sample = ctx->in_filled_sample *
						ctx->out_params.sample_rate /
						ctx->in_params.sample_rate;
			if (ctx->in_filled_sample * ctx->out_params.sample_rate
					% ctx->in_params.sample_rate != 0)
				ctx->out_missed_sample += 1;

			ret = write_pf_coeff_mem(easrc, i,
						 ctx->st1_coeff,
						 ctx->st1_num_taps,
						 ctx->st1_addexp);
			if (ret)
				goto skip_load;

			ret = write_pf_coeff_mem(easrc, i,
						 ctx->st2_coeff,
						 ctx->st2_num_taps,
						 ctx->st2_addexp);
			if (ret)
				goto skip_load;
		}
	}

	LOG("easrc resuemed!\n");
skip_load:
	return;
}

void easrc_dump(volatile void *asrc_addr)
{
}
