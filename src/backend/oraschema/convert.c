#include "postgres.h"
#include "fmgr.h"
#include "lib/stringinfo.h"
#include "mb/pg_wchar.h"
#include "utils/builtins.h"
#include "utils/int8.h"
#include "utils/numeric.h"
#include "utils/pg_locale.h"
#include "utils/formatting.h"

#include "oraschema.h"

extern char *nls_date_format;
extern char *nls_timestamp_format;
extern char *nls_timestamp_tz_format;

static int getindex(const char **map, char *mbchar, int mblen);

#define RETURN_IF_WITHOUT_FORMAT(out_func) \
	do { \
		if (!fmt) \
		{ \
			Datum result; \
			result = DirectFunctionCall1(out_func, PG_GETARG_DATUM(0)); \
			PG_RETURN_TEXT_P(cstring_to_text(DatumGetCString(result))); \
		} \
	} while(0)

#define RETURN_NULL_IF_EMPTY_INPUT(in) \
	do { \
		if (in) \
		{ \
			char instr[2]= {0, 0}; \
			text_to_cstring_buffer(in, instr, 2); \
			if (instr[0] == '\0') \
				PG_RETURN_NULL(); \
		} \
	} while (0)

Datum
int4_tochar(PG_FUNCTION_ARGS)
{
	text		*fmt = PG_GETARG_TEXT_P_IF_NULL(1);

	RETURN_IF_WITHOUT_FORMAT(int4out);

	RETURN_NULL_IF_EMPTY_INPUT(fmt);

	return DirectFunctionCall2(int4_to_char,
								PG_GETARG_DATUM(0),
								PG_GETARG_DATUM(1));
}

Datum
int8_tochar(PG_FUNCTION_ARGS)
{
	text		*fmt = PG_GETARG_TEXT_P_IF_NULL(1);

	RETURN_IF_WITHOUT_FORMAT(int8out);

	RETURN_NULL_IF_EMPTY_INPUT(fmt);

	return DirectFunctionCall2(int8_to_char,
								PG_GETARG_DATUM(0),
								PG_GETARG_DATUM(1));
}

Datum
float4_tochar(PG_FUNCTION_ARGS)
{
	text		*fmt = PG_GETARG_TEXT_P_IF_NULL(1);

	RETURN_IF_WITHOUT_FORMAT(float4out);

	RETURN_NULL_IF_EMPTY_INPUT(fmt);

	return DirectFunctionCall2(float4_to_char,
								PG_GETARG_DATUM(0),
								PG_GETARG_DATUM(1));
}

Datum
float8_tochar(PG_FUNCTION_ARGS)
{
	text	   *fmt = PG_GETARG_TEXT_P_IF_NULL(1);

	RETURN_IF_WITHOUT_FORMAT(float8out);

	RETURN_NULL_IF_EMPTY_INPUT(fmt);

	return DirectFunctionCall2(float8_to_char,
								PG_GETARG_DATUM(0),
								PG_GETARG_DATUM(1));
}

Datum
numeric_tochar(PG_FUNCTION_ARGS)
{
	text	   *fmt = PG_GETARG_TEXT_P_IF_NULL(1);

	RETURN_IF_WITHOUT_FORMAT(numeric_out);

	RETURN_NULL_IF_EMPTY_INPUT(fmt);

	return DirectFunctionCall2(numeric_to_char,
								PG_GETARG_DATUM(0),
								PG_GETARG_DATUM(1));
}

Datum
text_tochar(PG_FUNCTION_ARGS)
{
	return PG_GETARG_DATUM(0);
}

Datum
timestamp_tochar(PG_FUNCTION_ARGS)
{
	text *fmt = PG_GETARG_TEXT_P_IF_NULL(1);

	RETURN_NULL_IF_EMPTY_INPUT(fmt);

	if (!fmt)
	{
		if (nls_timestamp_format && strlen(nls_timestamp_format))
			fmt = cstring_to_text(nls_timestamp_format);
		else
			PG_RETURN_NULL();
	}

	return DirectFunctionCall2(timestamp_to_char,
								PG_GETARG_DATUM(0),
								PointerGetDatum(fmt));
}

Datum
timestamptz_tochar(PG_FUNCTION_ARGS)
{
	text *fmt = PG_GETARG_TEXT_P_IF_NULL(1);

	RETURN_NULL_IF_EMPTY_INPUT(fmt);

	if (!fmt)
	{
		if (nls_timestamp_tz_format && strlen(nls_timestamp_tz_format))
			fmt = cstring_to_text(nls_timestamp_tz_format);
		else
			PG_RETURN_NULL();
	}

	return DirectFunctionCall2(timestamptz_to_char,
								PG_GETARG_DATUM(0),
								PointerGetDatum(fmt));
}

Datum
interval_tochar(PG_FUNCTION_ARGS)
{
	text *fmt = PG_GETARG_TEXT_P_IF_NULL(1);

	RETURN_NULL_IF_EMPTY_INPUT(fmt);

	if (!fmt)
	{
		if (nls_timestamp_format && strlen(nls_timestamp_format))
			fmt = cstring_to_text(nls_timestamp_format);
		else
			PG_RETURN_NULL();
	}

	return DirectFunctionCall2(interval_to_char,
								PG_GETARG_DATUM(0),
								PointerGetDatum(fmt));
}

Datum
trunc_text_toint2(PG_FUNCTION_ARGS)
{
	text	*txt = PG_GETARG_TEXT_PP_IF_NULL(0);
	char	*txtstr = NULL;
	Datum	result;

	if (!txt)
		PG_RETURN_NULL();

	txtstr = text_to_cstring(txt);
	PG_TRY();
	{
		volatile bool err = false;

		/* first call int2in to convert text to int2 */
		PG_TRY_HOLD();
		{
			result = DirectFunctionCall1(int2in, CStringGetDatum(txtstr));
		} PG_CATCH_HOLD();
		{
			errdump();
			err = true;
		} PG_END_TRY_HOLD();

		/* second try to trunc(text::numeric)::int2 */
		if (err)
		{
			/* 1.text -> numeric */
			Datum txtnum = DirectFunctionCall3(numeric_in,
												CStringGetDatum(txtstr),
												0,
												-1);
			/* 2. trunc(txtnum) */
			txtnum = DirectFunctionCall2(numeric_trunc,
										 txtnum,
										 Int32GetDatum(0));

			/* 3. numeric -> int2 */
			result = DirectFunctionCall1(numeric_int2,
										 txtnum);
		}
	} PG_CATCH();
	{
		pfree(txtstr);
		PG_RE_THROW();
	} PG_END_TRY();
	pfree(txtstr);

	return result;
}

Datum
trunc_text_toint4(PG_FUNCTION_ARGS)
{
	text	*txt = PG_GETARG_TEXT_PP_IF_NULL(0);
	char	*txtstr = NULL;
	Datum	result;

	if (!txt)
		PG_RETURN_NULL();

	txtstr = text_to_cstring(txt);
	PG_TRY();
	{
		volatile bool err = false;

		/* first call int4in to convert text to int4 */
		PG_TRY_HOLD();
		{
			result = DirectFunctionCall1(int4in, CStringGetDatum(txtstr));
		} PG_CATCH_HOLD();
		{
			errdump();
			err = true;
		} PG_END_TRY_HOLD();

		/* second try to trunc(text::numeric)::int4 */
		if (err)
		{
			/* 1.text -> numeric */
			Datum txtnum = DirectFunctionCall3(numeric_in,
												CStringGetDatum(txtstr),
												0,
												-1);
			/* 2. trunc(txtnum) */
			txtnum = DirectFunctionCall2(numeric_trunc,
										 txtnum,
										 Int32GetDatum(0));

			/* 3. numeric -> int4 */
			result = DirectFunctionCall1(numeric_int4,
										 txtnum);
		}
	} PG_CATCH();
	{
		pfree(txtstr);
		PG_RE_THROW();
	} PG_END_TRY();
	pfree(txtstr);

	return result;
}

Datum
trunc_text_toint8(PG_FUNCTION_ARGS)
{
	text	*txt = PG_GETARG_TEXT_PP_IF_NULL(0);
	char	*txtstr = NULL;
	Datum	result;

	if (!txt)
		PG_RETURN_NULL();

	txtstr = text_to_cstring(txt);
	PG_TRY();
	{
		volatile bool err = false;

		/* first call int8in to convert text to int8 */
		PG_TRY_HOLD();
		{
			result = DirectFunctionCall1(int8in, CStringGetDatum(txtstr));
		} PG_CATCH_HOLD();
		{
			errdump();
			err = true;
		} PG_END_TRY_HOLD();

		/* second try to trunc(text::numeric)::int8 */
		if (err)
		{
			/* 1.text -> numeric */
			Datum txtnum = DirectFunctionCall3(numeric_in,
												CStringGetDatum(txtstr),
												0,
												-1);
			/* 2. trunc(txtnum) */
			txtnum = DirectFunctionCall2(numeric_trunc,
										 txtnum,
										 Int32GetDatum(0));

			/* 3. numeric -> int2 */
			result = DirectFunctionCall1(numeric_int8,
										 txtnum);
		}
	} PG_CATCH();
	{
		pfree(txtstr);
		PG_RE_THROW();
	} PG_END_TRY();
	pfree(txtstr);

	return result;
}

Datum
text_toint2(PG_FUNCTION_ARGS)
{
	text	*txt = PG_GETARG_TEXT_PP_IF_NULL(0);
	char	*txtstr = NULL;
	Datum	result;

	if (!txt)
		PG_RETURN_NULL();

	txtstr = text_to_cstring(txt);
	result = DirectFunctionCall1(int2in, CStringGetDatum(txtstr));
	pfree(txtstr);

	return result;
}

Datum
text_toint4(PG_FUNCTION_ARGS)
{
	text	*txt = PG_GETARG_TEXT_PP_IF_NULL(0);
	char	*txtstr = NULL;
	Datum	result;

	if (!txt)
		PG_RETURN_NULL();

	txtstr = text_to_cstring(txt);
	result = DirectFunctionCall1(int4in, CStringGetDatum(txtstr));
	pfree(txtstr);

	return result;
}

Datum
text_toint8(PG_FUNCTION_ARGS)
{
	text	*txt = PG_GETARG_TEXT_PP_IF_NULL(0);
	char	*txtstr = NULL;
	Datum	result;

	if (!txt)
		PG_RETURN_NULL();

	txtstr = text_to_cstring(txt);
	result = DirectFunctionCall1(int8in, CStringGetDatum(txtstr));
	pfree(txtstr);

	return result;
}


Datum
text_tofloat4(PG_FUNCTION_ARGS)
{
	text	*txt = PG_GETARG_TEXT_PP_IF_NULL(0);
	char	*txtstr = NULL;
	Datum	result;

	if (!txt)
		PG_RETURN_NULL();

	txtstr = text_to_cstring(txt);
	result = DirectFunctionCall1(float4in,
								 CStringGetDatum(txtstr));
	pfree(txtstr);

	return result;
}

Datum
text_tofloat8(PG_FUNCTION_ARGS)
{
	text	*txt = PG_GETARG_TEXT_PP_IF_NULL(0);
	char	*txtstr = NULL;
	Datum	result;

	if (!txt)
		PG_RETURN_NULL();

	txtstr = text_to_cstring(txt);
	result = DirectFunctionCall1(float8in,
								 CStringGetDatum(txtstr));
	pfree(txtstr);

	return result;
}

Datum
text_todate(PG_FUNCTION_ARGS)
{
	text *txt = PG_GETARG_TEXT_P_IF_NULL(0);
	text *fmt = PG_GETARG_TEXT_P_IF_NULL(1);
	Datum result;

	/* return null if input txt is null */
	if (!txt)
		PG_RETURN_NULL();

	/* return null if input txt is empty */
	RETURN_NULL_IF_EMPTY_INPUT(txt);

	/* return null if input fmt is empty */
	RETURN_NULL_IF_EMPTY_INPUT(fmt);

	/* get default date format style */
	if (!fmt && nls_date_format && strlen(nls_date_format))
		fmt = cstring_to_text(nls_date_format);

	if(fmt)
	{
		/* it will return timestamp at GMT */
		result = DirectFunctionCall2(to_timestamp,
									  PG_GETARG_DATUM(0),
									  PointerGetDatum(fmt));
	}
	else
	{
		char *txtstr = text_to_cstring(txt);

		result = DirectFunctionCall3(timestamp_in,
									 CStringGetDatum(txtstr),
									 ObjectIdGetDatum(InvalidOid),
									 Int32GetDatum(0));
		pfree(txtstr);
	}

	return result;
}

Datum
text_totimestamp(PG_FUNCTION_ARGS)
{
	text *txt = PG_GETARG_TEXT_P_IF_NULL(0);
	text *fmt = PG_GETARG_TEXT_P_IF_NULL(1);
	Datum result;

	/* return null if input txt is null */
	if (!txt)
		PG_RETURN_NULL();

	/* return null if input txt is empty */
	RETURN_NULL_IF_EMPTY_INPUT(txt);

	/* return null if input fmt is empty */
	RETURN_NULL_IF_EMPTY_INPUT(fmt);

	/* get default date format style */
	if (!fmt && nls_timestamp_format && strlen(nls_timestamp_format))
		fmt = cstring_to_text(nls_timestamp_format);

	if(fmt)
	{
		/* it will return timestamp at GMT */
		result = DirectFunctionCall2(to_timestamp,
									  PG_GETARG_DATUM(0),
									  PointerGetDatum(fmt));

		/* it will return timestamp at local */
		result = DirectFunctionCall1(timestamptz_timestamp, result);
	}
	else
	{
		char *txtstr = text_to_cstring(txt);

		result = DirectFunctionCall3(timestamp_in,
									 CStringGetDatum(txtstr),
									 ObjectIdGetDatum(InvalidOid),
									 Int32GetDatum(-1));
		pfree(txtstr);
	}

	return result;
}

Datum
text_totimestamptz(PG_FUNCTION_ARGS)
{
	text *txt = PG_GETARG_TEXT_P_IF_NULL(0);
	text *fmt = PG_GETARG_TEXT_P_IF_NULL(1);
	Datum result;

	/* return null if input txt is null */
	if (!txt)
		PG_RETURN_NULL();

	/* return null if input txt is empty */
	RETURN_NULL_IF_EMPTY_INPUT(txt);

	/* return null if input fmt is empty */
	RETURN_NULL_IF_EMPTY_INPUT(fmt);

	/* get default date format style */
	if (!fmt && nls_timestamp_tz_format && strlen(nls_timestamp_tz_format))
		fmt = cstring_to_text(nls_timestamp_tz_format);

	if(fmt)
	{
		/* it will return timestamp at GMT */
		result = DirectFunctionCall2(to_timestamp,
									  PG_GETARG_DATUM(0),
									  PointerGetDatum(fmt));
	}
	else
	{
		char *txtstr = text_to_cstring(txt);

		result = DirectFunctionCall3(timestamptz_in,
									 CStringGetDatum(txtstr),
									 ObjectIdGetDatum(InvalidOid),
									 Int32GetDatum(0));
		pfree(txtstr);
	}

	return result;
}

Datum
text_tocstring(PG_FUNCTION_ARGS)
{
	text	*txt = PG_GETARG_TEXT_PP_IF_NULL(0);

	if (!txt)
		PG_RETURN_NULL();

	return CStringGetDatum(text_to_cstring(txt));
}

/*
 * oracle function to_number(text, text)
 * 		convert string to numeric
 */
Datum
text_tonumber(PG_FUNCTION_ARGS)
{
	text *txt = PG_GETARG_TEXT_PP_IF_NULL(0);
	text *fmt = PG_GETARG_TEXT_PP_IF_NULL(1);

	if (!txt)
		PG_RETURN_NULL();

	RETURN_NULL_IF_EMPTY_INPUT(txt);
	RETURN_NULL_IF_EMPTY_INPUT(fmt);

	if (!fmt)
	{
		Datum result;
		char *txtstr = text_to_cstring(txt);
		result = DirectFunctionCall3(numeric_in,
								   CStringGetDatum(txtstr),
								   0,
								   -1);
		pfree(txtstr);

		return result;
	} else
	{
		return DirectFunctionCall2(numeric_to_number,
							PG_GETARG_DATUM(0),
							PG_GETARG_DATUM(1));
	}
}

Datum
float4_tonumber(PG_FUNCTION_ARGS)
{
	text *fmt = PG_GETARG_TEXT_PP_IF_NULL(1);

	RETURN_NULL_IF_EMPTY_INPUT(fmt);

	if (!fmt)
	{
		return DirectFunctionCall1(float4_numeric, PG_GETARG_DATUM(0));
	} else
	{
		Datum val = DirectFunctionCall1(float4out, PG_GETARG_DATUM(0));
		text *valtxt = cstring_to_text(DatumGetCString(val));

		return DirectFunctionCall2(numeric_to_number,
									PointerGetDatum(valtxt),
									PG_GETARG_DATUM(1));
	}
}

Datum
float8_tonumber(PG_FUNCTION_ARGS)
{
	text *fmt = PG_GETARG_TEXT_PP_IF_NULL(1);

	RETURN_NULL_IF_EMPTY_INPUT(fmt);

	if (!fmt)
	{
		return DirectFunctionCall1(float8_numeric, PG_GETARG_DATUM(0));
	} else
	{
		Datum val = DirectFunctionCall1(float8out, PG_GETARG_DATUM(0));
		text *valtxt = cstring_to_text(DatumGetCString(val));

		return DirectFunctionCall2(numeric_to_number,
									PointerGetDatum(valtxt),
									PG_GETARG_DATUM(1));
	}
}

/* 3 is enough, but it is defined as 4 in backend code. */
#ifndef MAX_CONVERSION_GROWTH
#define MAX_CONVERSION_GROWTH  4
#endif

/*
 * Convert a tilde (~) to ...
 *	1: a full width tilde. (same as JA16EUCTILDE in oracle)
 *	0: a full width overline. (same as JA16EUC in oracle)
 */
#define JA_TO_FULL_WIDTH_TILDE	1

static const char *
TO_MULTI_BYTE_UTF8[95] =
{
	"\343\200\200",
	"\357\274\201",
	"\342\200\235",
	"\357\274\203",
	"\357\274\204",
	"\357\274\205",
	"\357\274\206",
	"\342\200\231",
	"\357\274\210",
	"\357\274\211",
	"\357\274\212",
	"\357\274\213",
	"\357\274\214",
	"\357\274\215",
	"\357\274\216",
	"\357\274\217",
	"\357\274\220",
	"\357\274\221",
	"\357\274\222",
	"\357\274\223",
	"\357\274\224",
	"\357\274\225",
	"\357\274\226",
	"\357\274\227",
	"\357\274\230",
	"\357\274\231",
	"\357\274\232",
	"\357\274\233",
	"\357\274\234",
	"\357\274\235",
	"\357\274\236",
	"\357\274\237",
	"\357\274\240",
	"\357\274\241",
	"\357\274\242",
	"\357\274\243",
	"\357\274\244",
	"\357\274\245",
	"\357\274\246",
	"\357\274\247",
	"\357\274\250",
	"\357\274\251",
	"\357\274\252",
	"\357\274\253",
	"\357\274\254",
	"\357\274\255",
	"\357\274\256",
	"\357\274\257",
	"\357\274\260",
	"\357\274\261",
	"\357\274\262",
	"\357\274\263",
	"\357\274\264",
	"\357\274\265",
	"\357\274\266",
	"\357\274\267",
	"\357\274\270",
	"\357\274\271",
	"\357\274\272",
	"\357\274\273",
	"\357\277\245",
	"\357\274\275",
	"\357\274\276",
	"\357\274\277",
	"\342\200\230",
	"\357\275\201",
	"\357\275\202",
	"\357\275\203",
	"\357\275\204",
	"\357\275\205",
	"\357\275\206",
	"\357\275\207",
	"\357\275\210",
	"\357\275\211",
	"\357\275\212",
	"\357\275\213",
	"\357\275\214",
	"\357\275\215",
	"\357\275\216",
	"\357\275\217",
	"\357\275\220",
	"\357\275\221",
	"\357\275\222",
	"\357\275\223",
	"\357\275\224",
	"\357\275\225",
	"\357\275\226",
	"\357\275\227",
	"\357\275\230",
	"\357\275\231",
	"\357\275\232",
	"\357\275\233",
	"\357\275\234",
	"\357\275\235",
#if JA_TO_FULL_WIDTH_TILDE
	"\357\275\236"
#else
	"\357\277\243"
#endif
};

static const char *
TO_MULTI_BYTE_EUCJP[95] =
{
	"\241\241",
	"\241\252",
	"\241\311",
	"\241\364",
	"\241\360",
	"\241\363",
	"\241\365",
	"\241\307",
	"\241\312",
	"\241\313",
	"\241\366",
	"\241\334",
	"\241\244",
	"\241\335",
	"\241\245",
	"\241\277",
	"\243\260",
	"\243\261",
	"\243\262",
	"\243\263",
	"\243\264",
	"\243\265",
	"\243\266",
	"\243\267",
	"\243\270",
	"\243\271",
	"\241\247",
	"\241\250",
	"\241\343",
	"\241\341",
	"\241\344",
	"\241\251",
	"\241\367",
	"\243\301",
	"\243\302",
	"\243\303",
	"\243\304",
	"\243\305",
	"\243\306",
	"\243\307",
	"\243\310",
	"\243\311",
	"\243\312",
	"\243\313",
	"\243\314",
	"\243\315",
	"\243\316",
	"\243\317",
	"\243\320",
	"\243\321",
	"\243\322",
	"\243\323",
	"\243\324",
	"\243\325",
	"\243\326",
	"\243\327",
	"\243\330",
	"\243\331",
	"\243\332",
	"\241\316",
	"\241\357",
	"\241\317",
	"\241\260",
	"\241\262",
	"\241\306",
	"\243\341",
	"\243\342",
	"\243\343",
	"\243\344",
	"\243\345",
	"\243\346",
	"\243\347",
	"\243\350",
	"\243\351",
	"\243\352",
	"\243\353",
	"\243\354",
	"\243\355",
	"\243\356",
	"\243\357",
	"\243\360",
	"\243\361",
	"\243\362",
	"\243\363",
	"\243\364",
	"\243\365",
	"\243\366",
	"\243\367",
	"\243\370",
	"\243\371",
	"\243\372",
	"\241\320",
	"\241\303",
	"\241\321",
#if JA_TO_FULL_WIDTH_TILDE
	"\241\301"
#else
	"\241\261"
#endif
};

Datum
ora_to_multi_byte(PG_FUNCTION_ARGS)
{
	text	   *src;
	text	   *dst;
	const char *s;
	char	   *d;
	int			srclen;
	int			dstlen;
	int			i;
	const char **map;

	switch (GetDatabaseEncoding())
	{
		case PG_UTF8:
			map = TO_MULTI_BYTE_UTF8;
			break;
		case PG_EUC_JP:
#if PG_VERSION_NUM >= 80300
		case PG_EUC_JIS_2004:
#endif
			map = TO_MULTI_BYTE_EUCJP;
			break;
		/*
		 * TODO: Add converter for encodings.
		 */
		default:	/* no need to convert */
			PG_RETURN_DATUM(PG_GETARG_DATUM(0));
	}

	src = PG_GETARG_TEXT_PP(0);
	s = VARDATA_ANY(src);
	srclen = VARSIZE_ANY_EXHDR(src);
	dst = (text *) palloc(VARHDRSZ + srclen * MAX_CONVERSION_GROWTH);
	d = VARDATA(dst);

	for (i = 0; i < srclen; i++)
	{
		unsigned char	u = (unsigned char) s[i];
		if (0x20 <= u && u <= 0x7e)
		{
			const char *m = map[u - 0x20];
			while (*m)
			{
				*d++ = *m++;
			}
		}
		else
		{
			*d++ = s[i];
		}
	}

	dstlen = d - VARDATA(dst);
	SET_VARSIZE(dst, VARHDRSZ + dstlen);

	PG_RETURN_TEXT_P(dst);
}

static int
getindex(const char **map, char *mbchar, int mblen)
{
	int		i;

	for (i = 0; i < 95; i++)
	{
		if (!memcmp(map[i], mbchar, mblen))
			return i;
	}

	return -1;
}

Datum
ora_to_single_byte(PG_FUNCTION_ARGS)
{
	text	   *src;
	text	   *dst;
	char	   *s;
	char	   *d;
	int			srclen;
	int			dstlen;
	const char **map;

	switch (GetDatabaseEncoding())
	{
		case PG_UTF8:
			map = TO_MULTI_BYTE_UTF8;
			break;
		case PG_EUC_JP:
#if PG_VERSION_NUM >= 80300
		case PG_EUC_JIS_2004:
#endif
			map = TO_MULTI_BYTE_EUCJP;
			break;
		/*
		 * TODO: Add converter for encodings.
		 */
		default:	/* no need to convert */
			PG_RETURN_DATUM(PG_GETARG_DATUM(0));
	}

	src = PG_GETARG_TEXT_PP(0);
	s = VARDATA_ANY(src);
	srclen = VARSIZE_ANY_EXHDR(src);

	/* XXX - The output length should be <= input length */
	dst = (text *) palloc0(VARHDRSZ + srclen);
	d = VARDATA(dst);

	while (*s && (s - VARDATA_ANY(src) < srclen))
	{
		char   *u = s;
		int		clen;
		int		mapindex;

		clen = pg_mblen(u);
		s += clen;

		if (clen == 1)
			*d++ = *u;
		else if ((mapindex = getindex(map, u, clen)) >= 0)
		{
			const char m = 0x20 + mapindex;
			*d++ = m;
		}
		else
		{
			memcpy(d, u, clen);
			d += clen;
		}
	}

	dstlen = d - VARDATA(dst);
	SET_VARSIZE(dst, VARHDRSZ + dstlen);

	PG_RETURN_TEXT_P(dst);
}
