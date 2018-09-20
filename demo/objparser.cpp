#define _CRT_SECURE_NO_WARNINGS

#include "objparser.h"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

template <typename T>
static void growArray(T*& data, size_t& capacity)
{
	size_t newcapacity = capacity == 0 ? 32 : capacity + capacity / 2;
	T* newdata = new T[newcapacity];

	if (data)
	{
		memcpy(newdata, data, capacity * sizeof(T));
		delete[] data;
	}

	data = newdata;
	capacity = newcapacity;
}

static int fixupIndex(int index, size_t size)
{
	return (index >= 0) ? index - 1 : int(size) + index;
}

static int parseInt(const char* s, const char** end)
{
	// skip whitespace
	while (*s == ' ' || *s == '\t')
		s++;

	// read sign bit
	int sign = (*s == '-');
	s += (*s == '-' || *s == '+');

	unsigned int result = 0;

	for (;;)
	{
		if (unsigned(*s - '0') < 10)
			result = result * 10 + (*s - '0');
		else
			break;

		s++;
	}

	// return end-of-string
	*end = s;

	return sign ? -int(result) : int(result);
}

static float parseFloat(const char* s, const char** end)
{
	static const double digits[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
	static const double powers[] = {1e0, 1e+1, 1e+2, 1e+3, 1e+4, 1e+5, 1e+6, 1e+7, 1e+8, 1e+9, 1e+10, 1e+11, 1e+12, 1e+13, 1e+14, 1e+15, 1e+16, 1e+17, 1e+18, 1e+19, 1e+20, 1e+21, 1e+22};

	// skip whitespace
	while (*s == ' ' || *s == '\t')
		s++;

	// read sign
	double sign = (*s == '-') ? -1 : 1;
	s += (*s == '-' || *s == '+');

	// read integer part
	double result = 0;
	int power = 0;

	while (unsigned(*s - '0') < 10)
	{
		result = result * 10 + digits[*s - '0'];
		s++;
	}

	// read fractional part
	if (*s == '.')
	{
		s++;

		while (unsigned(*s - '0') < 10)
		{
			result = result * 10 + digits[*s - '0'];
			s++;
			power--;
		}
	}

	// read exponent part
	if ((*s | ' ') == 'e')
	{
		s++;

		// read exponent sign
		int expsign = (*s == '-') ? -1 : 1;
		s += (*s == '-' || *s == '+');

		// read exponent
		int exppower = 0;

		while (unsigned(*s - '0') < 10)
		{
			exppower = exppower * 10 + (*s - '0');
			s++;
		}

		// done!
		power += expsign * exppower;
	}

	// return end-of-string
	*end = s;

	// note: this is precise if result < 9e15
	// for longer inputs we lose a bit of precision here
	if (unsigned(-power) < sizeof(powers) / sizeof(powers[0]))
		return float(sign * result / powers[-power]);
	else if (unsigned(power) < sizeof(powers) / sizeof(powers[0]))
		return float(sign * result * powers[power]);
	else
		return float(sign * result * pow(10.0, power));
}

static const char* parseFace(const char* s, int& vi, int& vti, int& vni)
{
	while (*s == ' ' || *s == '\t')
		s++;

	vi = parseInt(s, &s);

	if (*s != '/')
		return s;
	s++;

	// handle vi//vni indices
	if (*s != '/')
		vti = parseInt(s, &s);

	if (*s != '/')
		return s;
	s++;

	vni = parseInt(s, &s);

	return s;
}

ObjFile::ObjFile()
    : v(0)
    , v_size(0)
    , v_cap(0)
    , vt(0)
    , vt_size(0)
    , vt_cap(0)
    , vn(0)
    , vn_size(0)
    , vn_cap(0)
    , fv(0)
    , fv_size(0)
    , fv_cap(0)
    , f(0)
    , f_size(0)
    , f_cap(0)
{
}

ObjFile::~ObjFile()
{
	delete[] v;
	delete[] vt;
	delete[] vn;
	delete[] fv;
	delete[] f;
}

void objParseLine(ObjFile& result, const char* line)
{
	if (line[0] == 'v' && line[1] == ' ')
	{
		const char* s = line + 2;

		float x = parseFloat(s, &s);
		float y = parseFloat(s, &s);
		float z = parseFloat(s, &s);

		if (result.v_size + 3 > result.v_cap)
			growArray(result.v, result.v_cap);

		result.v[result.v_size++] = x;
		result.v[result.v_size++] = y;
		result.v[result.v_size++] = z;
	}
	else if (line[0] == 'v' && line[1] == 't' && line[2] == ' ')
	{
		const char* s = line + 3;

		float u = parseFloat(s, &s);
		float v = parseFloat(s, &s);
		float w = parseFloat(s, &s);

		if (result.vt_size + 3 > result.vt_cap)
			growArray(result.vt, result.vt_cap);

		result.vt[result.vt_size++] = u;
		result.vt[result.vt_size++] = v;
		result.vt[result.vt_size++] = w;
	}
	else if (line[0] == 'v' && line[1] == 'n' && line[2] == ' ')
	{
		const char* s = line + 3;

		float x = parseFloat(s, &s);
		float y = parseFloat(s, &s);
		float z = parseFloat(s, &s);

		if (result.vn_size + 3 > result.vn_cap)
			growArray(result.vn, result.vn_cap);

		result.vn[result.vn_size++] = x;
		result.vn[result.vn_size++] = y;
		result.vn[result.vn_size++] = z;
	}
	else if (line[0] == 'f' && line[1] == ' ')
	{
		const char* s = line + 2;
		int fv = 0;

		size_t v = result.v_size / 3;
		size_t vt = result.vt_size / 3;
		size_t vn = result.vn_size / 3;

		while (*s)
		{
			int vi = 0, vti = 0, vni = 0;
			s = parseFace(s, vi, vti, vni);

			if (vi == 0)
				break;

			if (result.f_size + 3 > result.f_cap)
				growArray(result.f, result.f_cap);

			result.f[result.f_size++] = fixupIndex(vi, v);
			result.f[result.f_size++] = fixupIndex(vti, vt);
			result.f[result.f_size++] = fixupIndex(vni, vn);

			fv++;
		}

		if (result.fv_size + 1 > result.fv_cap)
			growArray(result.fv, result.fv_cap);

		result.fv[result.fv_size++] = char(fv);
	}
}

bool objParseFile(ObjFile& result, const char* path)
{
	FILE* file = fopen(path, "rb");
	if (!file)
		return false;

	char buffer[65536];
	size_t size = 0;

	while (!feof(file))
	{
		size += fread(buffer + size, 1, sizeof(buffer) - size, file);

		size_t line = 0;

		while (line < size)
		{
			// find the end of current line
			void* eol = memchr(buffer + line, '\n', size - line);
			if (!eol)
				break;

			// zero-terminate for objParseLine
			size_t next = static_cast<char*>(eol) - buffer;

			buffer[next] = 0;

			// process next line
			objParseLine(result, buffer + line);

			line = next + 1;
		}

		// move prefix of the last line in the buffer to the beginning of the buffer for next iteration
		assert(line <= size);

		memmove(buffer, buffer + line, size - line);
		size -= line;
	}

	if (size)
	{
		// process last line
		assert(size < sizeof(buffer));
		buffer[size] = 0;

		objParseLine(result, buffer);
	}

	fclose(file);
	return true;
}

bool objValidate(const ObjFile& result)
{
	size_t total_indices = 0;

	for (size_t face = 0; face < result.fv_size; ++face)
	{
		int fv = result.fv[face];

		if (fv < 3)
			return false;

		total_indices += fv;
	}

	if (total_indices * 3 != result.f_size)
		return false;

	size_t v = result.v_size / 3;
	size_t vt = result.vt_size / 3;
	size_t vn = result.vn_size / 3;

	for (size_t i = 0; i < result.f_size; i += 3)
	{
		int vi = result.f[i + 0];
		int vti = result.f[i + 1];
		int vni = result.f[i + 2];

		if (vi < 0)
			return false;

		if (vi >= 0 && size_t(vi) >= v)
			return false;

		if (vti >= 0 && size_t(vti) >= vt)
			return false;

		if (vni >= 0 && size_t(vni) >= vn)
			return false;
	}

	return true;
}

void objTriangulate(ObjFile& result)
{
	size_t total_indices = 0;

	for (size_t face = 0; face < result.fv_size; ++face)
	{
		int fv = result.fv[face];

		assert(fv >= 3);
		total_indices += (fv - 2) * 3;
	}

	int* f = new int[total_indices * 3];

	size_t read = 0;
	size_t write = 0;

	for (size_t face = 0; face < result.fv_size; ++face)
	{
		int fv = result.fv[face];

		for (int i = 0; i + 2 < fv; ++i)
		{
			f[write + 0] = result.f[read + 0];
			f[write + 1] = result.f[read + 1];
			f[write + 2] = result.f[read + 2];

			f[write + 3] = result.f[read + i * 3 + 3];
			f[write + 4] = result.f[read + i * 3 + 4];
			f[write + 5] = result.f[read + i * 3 + 5];

			f[write + 6] = result.f[read + i * 3 + 6];
			f[write + 7] = result.f[read + i * 3 + 7];
			f[write + 8] = result.f[read + i * 3 + 8];

			write += 9;
		}

		read += fv * 3;
	}

	assert(read == result.f_size);
	assert(write == total_indices * 3);

	delete[] result.f;
	result.f = f;
	result.f_size = result.f_cap = write;

	char* fv = new char[total_indices / 3];
	memset(fv, 3, total_indices / 3);

	delete[] result.fv;
	result.fv = fv;
	result.fv_size = result.fv_cap = total_indices / 3;
}
