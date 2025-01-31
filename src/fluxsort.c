/*
	Copyright (C) 2014-2021 Igor van den Hoven ivdhoven@gmail.com
*/

/*
	Permission is hereby granted, free of charge, to any person obtaining
	a copy of this software and associated documentation files (the
	"Software"), to deal in the Software without restriction, including
	without limitation the rights to use, copy, modify, merge, publish,
	distribute, sublicense, and/or sell copies of the Software, and to
	permit persons to whom the Software is furnished to do so, subject to
	the following conditions:

	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
	IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
	CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
	TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
	SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

/*
	fluxsort 1.1.4.3
*/

#define FLUX_OUT 24

size_t FUNC(flux_analyze)(VAR *array, size_t nmemb, CMPFUNC *cmp)
{
	size_t cnt, balance = 0;
	VAR *pta, *ptb, swap;

	pta = array;
	cnt = nmemb;

	while (--cnt)
	{
		balance += cmp(pta, pta + 1) > 0;
		pta++;
	}

	if (balance == 0)
	{
		return 1;
	}

	if (balance == nmemb - 1)
	{
		pta = array;
		ptb = array + nmemb;

		cnt = nmemb / 2;

		do
		{
			swap = *pta; *pta++ = *--ptb; *ptb = swap;
		}
		while (--cnt);

		return 1;
	}

	if (balance <= nmemb / 6 || balance >= nmemb / 6 * 5)
	{
		FUNC(quadsort)(array, nmemb, cmp);

		return 1;
	}
	return 0;
}

static size_t FUNC(median_of_five)(VAR *array, size_t v0, size_t v1, size_t v2, size_t v3, size_t v4, CMPFUNC *cmp)
{
	unsigned char t[4], val;

	val = cmp(&array[v0], &array[v1]) > 0; t[0]  = val; t[1] = !val;
	val = cmp(&array[v0], &array[v2]) > 0; t[0] += val; t[2] = !val;
	val = cmp(&array[v0], &array[v3]) > 0; t[0] += val; t[3] = !val;
	val = cmp(&array[v0], &array[v4]) > 0; t[0] += val;

	if (t[0] == 2) return v0;

	val = cmp(&array[v1], &array[v2]) > 0; t[1] += val; t[2] += !val;
	val = cmp(&array[v1], &array[v3]) > 0; t[1] += val; t[3] += !val;
	val = cmp(&array[v1], &array[v4]) > 0; t[1] += val;

	if (t[1] == 2) return v1;

	val = cmp(&array[v2], &array[v3]) > 0; t[2] += val; t[3] += !val;
	val = cmp(&array[v2], &array[v4]) > 0; t[2] += val;

	if (t[2] == 2) return v2;

	val = cmp(&array[v3], &array[v4]) > 0; t[3] += val;

	return t[3] == 2 ? v3 : v4;
}

static size_t FUNC(median_of_three)(VAR *array, size_t v0, size_t v1, size_t v2, CMPFUNC *cmp)
{
	unsigned char t[2], val;

	val = cmp(&array[v0], &array[v1]) > 0; t[0]  = val; t[1] = !val;
	val = cmp(&array[v0], &array[v2]) > 0; t[0] += val;

	if (t[0] == 1) return v0;

	val = cmp(&array[v1], &array[v2]) > 0; t[1] += val;

	return t[1] == 1 ? v1 : v2;
}

static size_t FUNC(median_of_three_rand)(VAR *array, size_t start, size_t inc, unsigned *seedpnt, unsigned mask, CMPFUNC *cmp)
{
	size_t v0, v1, v2;
	unsigned seed = *seedpnt;

	v0 = start + (seed & mask);
	seed ^= seed << 7;
	start += inc;
	v1 = start + (seed & mask);
	seed ^= seed >> 9;
	start += inc;
	v2 = start + (seed & mask);
	seed ^= seed << 8;

	*seedpnt = seed;

	return FUNC(median_of_three)(array, v1, v0, v2, cmp);
}

VAR FUNC(median_of_fifteen)(VAR *array, size_t nmemb, CMPFUNC *cmp)
{
	size_t v0, v1, v2, v3, v4, div = nmemb / 16;

	unsigned seed = nmemb;
	v0 = FUNC(median_of_three_rand)(array, div *  0, div, &seed, 63, cmp);
	v1 = FUNC(median_of_three_rand)(array, div *  3, div, &seed, 63, cmp);
	v2 = FUNC(median_of_three_rand)(array, div *  7, div, &seed, 63, cmp);
	v3 = FUNC(median_of_three_rand)(array, div * 10, div, &seed, 63, cmp);
	v4 = FUNC(median_of_three_rand)(array, div * 13, div, &seed, 63, cmp);

	return array[FUNC(median_of_five)(array, v2, v0, v1, v3, v4, cmp)];
}

VAR FUNC(median_of_nine)(VAR *array, size_t nmemb, CMPFUNC *cmp)
{
	size_t v0, v1, v2, div = nmemb / 16;

	v0 = FUNC(median_of_three)(array, div * 2, div * 1, div * 4, cmp);
	v1 = FUNC(median_of_three)(array, div * 8, div * 6, div * 10, cmp);
	v2 = FUNC(median_of_three)(array, div * 14, div * 12, div * 15, cmp);

	return array[FUNC(median_of_three)(array, v1, v0, v2, cmp)];
}

static size_t FUNC(flux_partition)(VAR *ptx, VAR *pte, VAR *pta, VAR *pts, VAR piv, CMPFUNC *cmp)
{
	unsigned char val;
	VAR *swap;

	swap = pts;

	while (ptx + 8 <= pte)
	{
		size_t n = 0;

		val = cmp(ptx + 0, &piv) <= 0; pta[n] = pts[0 - n] = ptx[0]; n += val;
		val = cmp(ptx + 1, &piv) <= 0; pta[n] = pts[1 - n] = ptx[1]; n += val;
		val = cmp(ptx + 2, &piv) <= 0; pta[n] = pts[2 - n] = ptx[2]; n += val;
		val = cmp(ptx + 3, &piv) <= 0; pta[n] = pts[3 - n] = ptx[3]; n += val;
		val = cmp(ptx + 4, &piv) <= 0; pta[n] = pts[4 - n] = ptx[4]; n += val;
		val = cmp(ptx + 5, &piv) <= 0; pta[n] = pts[5 - n] = ptx[5]; n += val;
		val = cmp(ptx + 6, &piv) <= 0; pta[n] = pts[6 - n] = ptx[6]; n += val;
		val = cmp(ptx + 7, &piv) <= 0; pta[n] = pts[7 - n] = ptx[7]; n += val;

		pta += n;
		pts += 8 - n;
		ptx += 8;
	}

	while (ptx < pte)
	{
		val = cmp(ptx, &piv) <= 0; pta[0] = pts[0] = ptx[0];

		pta += val;
		pts += !val;
		ptx++;
	}

	return pts - swap;
}

static size_t FUNC(flux_reverse_partition)(VAR *array, VAR *swap, VAR *ptx, VAR piv, size_t nmemb, CMPFUNC *cmp)
{
	unsigned char val;
	VAR *pta, *pts, *pte;

	pte = ptx;
	ptx = pte + nmemb;
	pts = swap + nmemb - 1;
	pta = array + nmemb - 1;

	while (ptx - 8 >= pte)
	{
		ptx -= 8;
		size_t n = 0;

		val = cmp(&piv, ptx + 7) <= 0; pta[-n] = pts[-0 + n] = ptx[7]; n += val;
		val = cmp(&piv, ptx + 6) <= 0; pta[-n] = pts[-1 + n] = ptx[6]; n += val;
		val = cmp(&piv, ptx + 5) <= 0; pta[-n] = pts[-2 + n] = ptx[5]; n += val;
		val = cmp(&piv, ptx + 4) <= 0; pta[-n] = pts[-3 + n] = ptx[4]; n += val;
		val = cmp(&piv, ptx + 3) <= 0; pta[-n] = pts[-4 + n] = ptx[3]; n += val;
		val = cmp(&piv, ptx + 2) <= 0; pta[-n] = pts[-5 + n] = ptx[2]; n += val;
		val = cmp(&piv, ptx + 1) <= 0; pta[-n] = pts[-6 + n] = ptx[1]; n += val;
		val = cmp(&piv, ptx + 0) <= 0; pta[-n] = pts[-7 + n] = ptx[0]; n += val;

		pta -= n;
		pts -= 8 - n;
	}

	while (ptx > pte)
	{
		ptx--;
		val = cmp(&piv, ptx) <= 0; pta[0] = pts[0] = ptx[0];

		pta -= val;
		pts -= !val;
	}

	return pta + 1 - array;
}

void FUNC(fluxsort)(void *array, size_t nmemb, CMPFUNC *cmp);

void FUNC(flux_loop)(VAR *array, VAR *swap, VAR *ptx, size_t nmemb, bool hasbound, CMPFUNC *cmp)
{
	size_t a_size, s_size;
	VAR *pta, *pts, *pte, piv;

recurse:
	if (nmemb <= 1024)
	{
		piv = FUNC(median_of_nine)(ptx, nmemb, cmp);
	}
	else if (nmemb <= 16384)
	{
		piv = FUNC(median_of_fifteen)(ptx, nmemb, cmp);
	}
	else
	{
		size_t log2, nt, div, cnt, i;
		unsigned seed, mask;
		VAR *tmp = ptx==array ? swap : array;

		log2 = 14;
		nt = nmemb >> log2;
		while (nt)
		{
			log2++;
			nt >>= 1;
		}
		div = (size_t)sqrt((double)nmemb * (2 * (2 + log2)));

		seed = nmemb;
		mask = (1 << (2 + log2 / 2)) - 1;
		for (cnt = i = 0 ; i < nmemb - (mask + 2 * div); )
		{
			seed ^= seed << 13;
			tmp[cnt++] = ptx[i + (seed & mask)];
			i += div;
			seed ^= seed >> 17;
			tmp[cnt++] = ptx[i + (seed & mask)];
			i += div;
			seed ^= seed << 5;
			tmp[cnt++] = ptx[i + (seed & mask)];
			i += div;
		}
		FUNC(fluxsort)(tmp, cnt, cmp);

		piv = tmp[cnt / 2];
	}

	if (hasbound && cmp(array + nmemb, &piv) <= 0)
	{
		s_size = 0;
	}
	else
	{
		s_size = FUNC(flux_partition)(ptx, ptx + nmemb, array, swap, piv, cmp);
		ptx = array;
	}

	if (s_size == 0)
	{
		VAR *pts = ptx != array ? ptx : swap;

		s_size = FUNC(flux_reverse_partition)(array, pts, ptx, piv, nmemb, cmp);
		pts += nmemb - s_size;

		if (s_size <= FLUX_OUT)
		{
			memcpy(array, pts, s_size * sizeof(VAR));
			FUNC(quadsort_swap)(array, swap, s_size, cmp);
		}
		else
		{
			ptx = pts;
			nmemb = s_size;
			goto recurse;
		}
	}
	else
	{
		a_size = nmemb - s_size;
		pta = array + a_size;

		if (a_size <= s_size / 16 || s_size <= FLUX_OUT)
		{
			memcpy(pta, swap, s_size * sizeof(VAR));
			FUNC(quadsort_swap)(pta, swap, s_size, cmp);
		}
		else
		{
			FUNC(flux_loop)(pta, swap, swap, s_size, hasbound, cmp);
		}
		hasbound = 1;

		if (s_size <= a_size / 16 || a_size <= FLUX_OUT)
		{
			FUNC(quadsort_swap)(array, swap, a_size, cmp);
		}
		else
		{
			nmemb = a_size;
			goto recurse;
		}
	}
}

void FUNC(fluxsort)(void *array, size_t nmemb, CMPFUNC *cmp)
{
	if (nmemb < 32)
	{
		FUNC(tail_swap)(array, nmemb, cmp);
	}
	else if (FUNC(flux_analyze)(array, nmemb, cmp) == 0)
	{
		VAR *swap = malloc(nmemb * sizeof(VAR));

		FUNC(flux_loop)(array, swap, array, nmemb, 0, cmp);

		free(swap);
	}
}
