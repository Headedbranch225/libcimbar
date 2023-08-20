/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include "chromatic_adaptation/nlargest.h"
#include <opencv2/opencv.hpp>

#include <tuple>

class Cell
{
public:
	static const bool SKIP = true;

public:
	Cell(const cv::Mat& img)
		: _img(img)
		, _cols(img.cols)
		, _rows(img.rows)
	{
	}

	Cell(const cv::Mat& img, int xstart, int ystart, int cols, int rows)
		: _img(img)
		, _xstart(xstart)
		, _ystart(ystart)
		, _cols(cols)
		, _rows(rows)
	{}

	// it would be nice to use a cropped cv::Mat to get the contiguous memory pointer...
	std::tuple<uchar,uchar,uchar> mean_rgb_continuous(bool skip, bool dark) const
	{
		uint16_t blue = 0;
		uint16_t green = 0;
		uint16_t red = 0;
		uint16_t count = 0;

		int channels = _img.channels();
		int index = (_ystart * _img.cols) + _xstart;
		const uchar* p = _img.ptr<uchar>(0) + (index * channels);

		int increment = 1 + skip;
		int toNextCol = channels * (_img.rows - _rows);
		if (skip)
			toNextCol += channels * _img.rows;

		for (int i = 0; i < _cols; i+=increment)
		{
			for (int j = 0; j < _rows; ++j, ++count)
			{
				red += p[0];
				green += p[1];
				blue += p[2];
				p += channels;
			}
			p += toNextCol;
		}

		if (!count)
			return std::tuple<uchar,uchar,uchar>(0, 0, 0);

		return std::tuple<uchar,uchar,uchar>(red/count, green/count, blue/count);
	}

	std::tuple<uchar,uchar,uchar> mean_rgb(bool skip=false, bool dark=true) const
	{
		int channels = _img.channels();
		if (channels < 3)
			return std::tuple<uchar,uchar,uchar>(0, 0, 0);
		if (_img.isContinuous() and _cols > 0)
			return mean_rgb_continuous(skip, dark);

		uint16_t blue = 0;
		uint16_t green = 0;
		uint16_t red = 0;
		uint16_t count = 0;

		int increment = 1 + skip;
		int yend = _img.rows * _img.channels();
		for (int i = 0; i < _img.cols; i+=increment)
		{
			const uchar* p = _img.ptr<uchar>(i);
			for (int j = 0; j < yend; j+=_img.channels(), ++count)
			{
				red += p[j];
				green += p[j+1];
				blue += p[j+2];
			}
		}

		if (!count)
			return std::tuple<uchar,uchar,uchar>(0, 0, 0);

		return std::tuple<uchar,uchar,uchar>(red/count, green/count, blue/count);
	}

	std::tuple<uchar,uchar,uchar> calc_rgb_continuous(bool skip) const
	{
		nlargest<uint8_t, 9> blues;
		nlargest<uint8_t, 9> greens;
		nlargest<uint8_t, 9> reds;

		int channels = _img.channels();
		int index = (_ystart * _img.cols) + _xstart;
		const uchar* p = _img.ptr<uchar>(0) + (index * channels);

		int increment = 1 + skip;
		int toNextCol = channels * (_img.rows - _rows);
		if (skip)
			toNextCol += channels * _img.rows;

		for (int i = 0; i < _cols; i+=increment)
		{
			for (int j = 0; j < _rows; ++j)
			{
				// skip pixel if don't we have enough "signal"
				reds.eval(p[0]);
				greens.eval(p[1]);
				blues.eval(p[2]);
				p += channels;
			}
			p += toNextCol;
		}

		int sk = 0;
		if (reds.nth(5) > 200 and reds.nth(5) > 200 and reds.nth(5) > 200)
		{
			std::cout << "everything is bright" << std::endl;
			sk = 3;
		}

		return std::tuple<uchar,uchar,uchar>(reds.mean(5, sk), greens.mean(5, sk), blues.mean(5, sk));
	}

	std::tuple<uchar,uchar,uchar> calc_rgb(bool skip=false) const
	{
		int channels = _img.channels();
		if (channels < 3)
			return std::tuple<uchar,uchar,uchar>(0, 0, 0);
		if (_img.isContinuous() and _cols > 0)
			return calc_rgb_continuous(skip);

		nlargest<uint8_t, 9> blues;
		nlargest<uint8_t, 9> greens;
		nlargest<uint8_t, 9> reds;

		int increment = 1 + skip;
		int yend = _img.rows * _img.channels();
		for (int i = 0; i < _img.cols; i+=increment)
		{
			const uchar* p = _img.ptr<uchar>(i);
			for (int j = 0; j < yend; j+=_img.channels())
			{
				reds.eval(p[j]);
				greens.eval(p[j+1]);
				blues.eval(p[j+2]);
			}
		}

		int sk = 0;
		if (reds.nth(5) > 200 and reds.nth(5) > 200 and reds.nth(5) > 200)
		{
			std::cout << "everything is bright" << std::endl;
			sk = 3;
		}

		return std::tuple<uchar,uchar,uchar>(reds.mean(5, sk), greens.mean(5, sk), blues.mean(5, sk));
	}

	uchar mean_grayscale_continuous() const
	{
		uint16_t total = 0;
		uint16_t count = 0;

		int index = (_ystart * _img.cols) + _xstart;
		const uchar* p = _img.ptr<uchar>(0) + index;
		int toNextCol = _img.rows - _rows;

		for (int i = 0; i < _img.cols; ++i)
		{
			for (int j = 0; j < _img.rows; ++j, ++count)
				total += p[count];
			count += toNextCol;
		}

		if (!count)
			return 0;
		return (uchar)(total/count);
	}

	uchar mean_grayscale() const
	{
		if (_img.channels() > 1)
			return 0;
		if (_img.isContinuous() and _cols > 0)
			return mean_grayscale_continuous();

		uint16_t total = 0;
		uint16_t count = 0;

		for (int i = 0; i < _img.cols; ++i)
		{
			const uchar* p = _img.ptr<uchar>(i);
			for (int j = 0; j < _img.rows; ++j, ++count)
				total += p[j];
		}

		if (!count)
			return 0;
		return (uchar)(total/count);
	}

	void crop(int x, int y, int cols, int rows)
	{
		_xstart += x;
		_ystart += y;
		_cols = cols;
		_rows = rows;
	}

	int cols() const
	{
		return _cols;
	}

	int rows() const
	{
		return _rows;
	}

protected:
	const cv::Mat& _img;
	int _xstart = 0;
	int _ystart = 0;
	int _cols;
	int _rows;
};
