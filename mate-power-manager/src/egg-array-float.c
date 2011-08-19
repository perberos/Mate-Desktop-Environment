/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2007-2008 Richard Hughes <richard@hughsie.com>
 *
 * Licensed under the GNU General Public License Version 2
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "config.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <glib.h>

#include "egg-debug.h"
#include "egg-array-float.h"

/**
 * egg_array_float_guassian_value:
 *
 * @x: input value
 * @sigma: sigma value
 * Return value: the gaussian, in floating point precision
 **/
static gfloat
egg_array_float_guassian_value (gfloat x, gfloat sigma)
{
	return (1.0 / (sqrtf(2.0*3.1415927) * sigma)) * (expf((-(powf(x,2.0)))/(2.0 * powf(sigma, 2.0))));
}

/**
 * egg_array_float_new:
 *
 * @length: length of array
 * Return value: Allocate array
 *
 * Creates a new size array which is zeroed. Free with g_array_free();
 **/
EggArrayFloat *
egg_array_float_new (guint length)
{
	guint i;
	EggArrayFloat *array;
	array = g_array_sized_new (TRUE, FALSE, sizeof(gfloat), length);
	array->len = length;

	/* clear to 0.0 */
	for (i=0; i<length; i++)
		g_array_index (array, gfloat, i) = 0.0;
	return array;
}

/**
 * egg_array_float_get:
 *
 * @array: input array
 **/
gfloat
egg_array_float_get (EggArrayFloat *array, guint i)
{
	if (i >= array->len)
		g_error ("above index! (%i)", i);
	return g_array_index (array, gfloat, i);
}

/**
 * egg_array_float_set:
 *
 * @array: input array
 **/
void
egg_array_float_set (EggArrayFloat *array, guint i, gfloat value)
{
	g_array_index (array, gfloat, i) = value;
}

/**
 * egg_array_float_free:
 *
 * @array: input array
 *
 * Frees the array, deallocating data
 **/
void
egg_array_float_free (EggArrayFloat *array)
{
	if (array != NULL)
		g_array_free (array, TRUE);
}

/**
 * egg_array_float_get_average:
 * @array: This class instance
 *
 * Gets the average value.
 **/
gfloat
egg_array_float_get_average (EggArrayFloat *array)
{
	guint i;
	guint length;
	gfloat average = 0;

	length = array->len;
	for (i=0; i<length; i++)
		average += g_array_index (array, gfloat, i);
	return average / (gfloat) length;
}

/**
 * egg_array_float_compute_gaussian:
 *
 * @length: length of output array
 * @sigma: sigma value
 * Return value: Gaussian array
 *
 * Create a set of Gaussian array of a specified size
 **/
EggArrayFloat *
egg_array_float_compute_gaussian (guint length, gfloat sigma)
{
	EggArrayFloat *array;
	guint half_length;
	guint i;
	gfloat division;
	gfloat value;

	g_return_val_if_fail (length % 2 == 1, NULL);

	array = egg_array_float_new (length);

	/* array positions 0..length, has to be an odd number */
	half_length = (length / 2) + 1;
	for (i=0; i<half_length; i++) {
		division = half_length - (i + 1);
	egg_debug ("half_length=%i, div=%f, sigma=%f", half_length, division, sigma);
		g_array_index (array, gfloat, i) = egg_array_float_guassian_value (division, sigma);
	}

	/* no point working these out, we can just reflect the gaussian */
	for (i=half_length; i<length; i++) {
		division = g_array_index (array, gfloat, length-(i+1));
		g_array_index (array, gfloat, i) = division;
	}

	/* make sure we get an accurate gaussian */
	value = egg_array_float_sum (array);
	if (fabs (value - 1.0f) > 0.01f) {
		egg_warning ("got wrong sum (%f), perhaps sigma too high for size?", value);
		egg_array_float_free (array);
		array = NULL;
	}

	return array;
}

/**
 * egg_array_float_sum:
 *
 * @array: input array
 *
 * Sum the elements of the array
 **/
gfloat
egg_array_float_sum (EggArrayFloat *array)
{
	guint length;
	guint i;
	gfloat total = 0;

	length = array->len;
	for (i=0; i<length; i++)
		total += g_array_index (array, gfloat, i);
	return total;
}

/**
 * egg_array_float_print:
 *
 * @array: input array
 *
 * Print the array
 **/
gboolean
egg_array_float_print (EggArrayFloat *array)
{
	guint length;
	guint i;

	length = array->len;
	/* debug out */
	for (i=0; i<length; i++)
		egg_debug ("[%i]\tval=%f", i, g_array_index (array, gfloat, i));
	return TRUE;
}

/**
 * egg_array_float_convolve:
 *
 * @data: input array
 * @kernel: kernel array
 * Return value: Colvolved array, same length as data
 *
 * Convolves an array with a kernel, and returns an array the same size.
 * THIS FUNCTION IS REALLY SLOW...
 **/
EggArrayFloat *
egg_array_float_convolve (EggArrayFloat *data, EggArrayFloat *kernel)
{
	gint length_data;
	gint length_kernel;
	EggArrayFloat *result;
	gfloat value;
	gint i;
	gint j;
	gint idx;

	length_data = data->len;
	length_kernel = kernel->len;

	result = egg_array_float_new (length_data);

	/* convolve */
	for (i=0;i<length_data;i++) {
		value = 0;
		for (j=0;j<length_kernel;j++) {
			idx = i+j-(length_kernel/2);
			if (idx < 0)
				idx = 0;
			else if (idx >= length_data)
				idx = length_data - 1;
			value += g_array_index (data, gfloat, idx) * g_array_index (kernel, gfloat, j);
		}
		g_array_index (result, gfloat, i) = value;
	}
	return result;
}

/**
 * egg_array_float_compute_integral:
 * @array: This class instance
 *
 * Computes complete discrete integral of dataset.
 * Will only work with a step size of one.
 **/
gfloat
egg_array_float_compute_integral (EggArrayFloat *array, guint x1, guint x2)
{
	gfloat value;
	guint i;

	g_return_val_if_fail (x2 >= x1, 0.0);

	/* if the same point, then we have no area */
	if (x1 == x2)
		return 0.0;

	value = 0.0;
	for (i=x1; i <= x2; i++)
		value += g_array_index (array, gfloat, i);
	return value;
}

/**
 * powfi:
 **/
static gfloat
powfi (gfloat base, guint n)
{
	guint i;
	gfloat retval = 1;
	for (i=1; i <= n; i++)
		retval *= base;
	return retval;
}

/**
 * egg_array_float_remove_outliers:
 *
 * @data: input array
 * @size: size to analyse
 * @sigma: sigma for standard deviation
 * Return value: Data with outliers removed
 *
 * Compares local sections of the data, removing outliers if they fall
 * ouside of sigma, and using the average of the other points in it's place.
 **/
EggArrayFloat *
egg_array_float_remove_outliers (EggArrayFloat *data, guint length, gfloat sigma)
{
	guint i;
	guint j;
	guint half_length;
	gfloat value;
	gfloat average;
	gfloat average_not_inc;
	gfloat average_square;
	gfloat biggest_difference;
	gfloat outlier_value;
	EggArrayFloat *result;

	g_return_val_if_fail (length % 2 == 1, NULL);
	result = egg_array_float_new (data->len);

	/* check for no data */
	if (data->len == 0)
		goto out;

	half_length = (length - 1) / 2;

	/* copy start and end of array */
	for (i=0; i < half_length; i++)
		g_array_index (result, gfloat, i) = g_array_index (data, gfloat, i);
	for (i=data->len-half_length; i < data->len; i++)
		g_array_index (result, gfloat, i) = g_array_index (data, gfloat, i);

	/* find the standard deviation of a block off data */
	for (i=half_length; i < data->len-half_length; i++) {
		average = 0;
		average_square = 0;

		/* find the average and the squared average */
		for (j=i-half_length; j<i+half_length+1; j++) {
			value = g_array_index (data, gfloat, j);
			average += value;
			average_square += powfi (value, 2);
		}

		/* divide by length to get average */
		average /= length;
		average_square /= length;

		/* find the standard deviation */
		value = sqrtf (average_square - powfi (average, 2));

		/* stddev is okay */
		if (value < sigma) {
			g_array_index (result, gfloat, i) = g_array_index (data, gfloat, i);
		} else {
			/* ignore the biggest difference from the average */
			biggest_difference = 0;
			outlier_value = 0;
			for (j=i-half_length; j<i+half_length+1; j++) {
				value = fabs (g_array_index (data, gfloat, j) - average);
				if (value > biggest_difference) {
					biggest_difference = value;
					outlier_value = g_array_index (data, gfloat, j);
				}
			}
			average_not_inc = (average * length) - outlier_value;
			average_not_inc /= length - 1;
			g_array_index (result, gfloat, i) = average_not_inc;
		}
	}
out:
	return result;
}

/***************************************************************************
 ***                          MAKE CHECK TESTS                           ***
 ***************************************************************************/
#ifdef EGG_TEST
#include "egg-test.h"

void
egg_array_float_test (gpointer data)
{
	EggArrayFloat *array;
	EggArrayFloat *kernel;
	EggArrayFloat *result;
	gfloat value;
	gfloat sigma;
	guint size;
	EggTest *test = (EggTest *) data;

	if (egg_test_start (test, "EggArrayFloat") == FALSE)
		return;

	/************************************************************/
	egg_test_title (test, "make sure we get a non null array");
	array = egg_array_float_new (10);
	if (array != NULL)
		egg_test_success (test, "got EggArrayFloat");
	else
		egg_test_failed (test, "could not get EggArrayFloat");
	egg_array_float_print (array);
	egg_array_float_free (array);

	/************************************************************/
	egg_test_title (test, "make sure we get the correct length array");
	array = egg_array_float_new (10);
	if (array->len == 10)
		egg_test_success (test, "got correct size");
	else
		egg_test_failed (test, "got wrong size");

	/************************************************************/
	egg_test_title (test, "make sure we get the correct array sum");
	value = egg_array_float_sum (array);
	if (value == 0.0)
		egg_test_success (test, "got correct sum");
	else
		egg_test_failed (test, "got wrong sum (%f)", value);

	/************************************************************/
	egg_test_title (test, "remove outliers");
	egg_array_float_set (array, 0, 30.0);
	egg_array_float_set (array, 1, 29.0);
	egg_array_float_set (array, 2, 31.0);
	egg_array_float_set (array, 3, 33.0);
	egg_array_float_set (array, 4, 100.0);
	egg_array_float_set (array, 5, 27.0);
	egg_array_float_set (array, 6, 30.0);
	egg_array_float_set (array, 7, 29.0);
	egg_array_float_set (array, 8, 31.0);
	egg_array_float_set (array, 9, 30.0);
	kernel = egg_array_float_remove_outliers (array, 3, 10.0);
	if (kernel != NULL && kernel->len == 10) 
		egg_test_success (test, "got correct length outlier array");
	else
		egg_test_failed (test, "got gaussian array length (%i)", array->len);
	egg_array_float_print (array);
	egg_array_float_print (kernel);

	/************************************************************/
	egg_test_title (test, "make sure we removed the outliers");
	value = egg_array_float_sum (kernel);
	if (fabs(value - 30*10) < 1)
		egg_test_success (test, "got sum (%f)", value);
	else
		egg_test_failed (test, "got wrong sum (%f)", value);
	egg_array_float_free (kernel);

	/************************************************************/
	egg_test_title (test, "remove outliers step");
	egg_array_float_set (array, 0, 0.0);
	egg_array_float_set (array, 1, 0.0);
	egg_array_float_set (array, 2, 0.0);
	egg_array_float_set (array, 3, 0.0);
	egg_array_float_set (array, 4, 0.0);
	egg_array_float_set (array, 5, 0.0);
	egg_array_float_set (array, 6, 0.0);
	egg_array_float_set (array, 7, 10.0);
	egg_array_float_set (array, 8, 20.0);
	egg_array_float_set (array, 9, 50.0);
	kernel = egg_array_float_remove_outliers (array, 3, 20.0);
	if (kernel != NULL && kernel->len == 10) 
		egg_test_success (test, "got correct length outlier array");
	else
		egg_test_failed (test, "got gaussian array length (%i)", array->len);
	egg_array_float_print (array);
	egg_array_float_print (kernel);

	/************************************************************/
	egg_test_title (test, "make sure we removed the outliers");
	value = egg_array_float_sum (kernel);
	if (fabs(value - 80) < 1)
		egg_test_success (test, "got sum (%f)", value);
	else
		egg_test_failed (test, "got wrong sum (%f)", value);
	egg_array_float_free (kernel);

	/************************************************************/
	egg_test_title (test, "get gaussian 0.0, sigma 1.1");
	value = egg_array_float_guassian_value (0.0, 1.1);
	if (fabs (value - 0.36267) < 0.0001)
		egg_test_success (test, "got correct gaussian");
	else
		egg_test_failed (test, "got wrong gaussian (%f)", value);

	/************************************************************/
	egg_test_title (test, "get gaussian 0.5, sigma 1.1");
	value = egg_array_float_guassian_value (0.5, 1.1);
	if (fabs (value - 0.32708) < 0.0001)
		egg_test_success (test, "got correct gaussian");
	else
		egg_test_failed (test, "got wrong gaussian (%f)", value);

	/************************************************************/
	egg_test_title (test, "get gaussian 1.0, sigma 1.1");
	value = egg_array_float_guassian_value (1.0, 1.1);
	if (fabs (value - 0.23991) < 0.0001)
		egg_test_success (test, "got correct gaussian");
	else
		egg_test_failed (test, "got wrong gaussian (%f)", value);

	/************************************************************/
	egg_test_title (test, "get gaussian 0.5, sigma 4.5");
	value = egg_array_float_guassian_value (0.5, 4.5);
	if (fabs (value - 0.088108) < 0.0001)
		egg_test_success (test, "got correct gaussian");
	else
		egg_test_failed (test, "got wrong gaussian (%f)", value);

	/************************************************************/
	size = 5;
	sigma = 1.1;
	egg_test_title (test, "get inprecise gaussian array (%i), sigma %f", size, sigma);
	kernel = egg_array_float_compute_gaussian (size, sigma);
	if (kernel == NULL) 
		egg_test_success (test, NULL);
	else {
		egg_test_failed (test, "got gaussian array length (%i)", array->len);
		egg_array_float_print (kernel);
	}

	/************************************************************/
	size = 9;
	sigma = 1.1;
	egg_test_title (test, "get gaussian-9 array (%i), sigma %f", size, sigma);
	kernel = egg_array_float_compute_gaussian (size, sigma);
	if (kernel != NULL && kernel->len == size) 
		egg_test_success (test, "got correct length gaussian array");
	else
		egg_test_failed (test, "got gaussian array length (%i)", array->len);
	egg_array_float_print (kernel);

	/************************************************************/
	egg_test_title (test, "make sure we get an accurate gaussian");
	value = egg_array_float_sum (kernel);
	if (fabs(value - 1.0) < 0.01)
		egg_test_success (test, "got sum (%f)", value);
	else
		egg_test_failed (test, "got wrong sum (%f)", value);

	/************************************************************/
	egg_test_title (test, "make sure we get get and set");
	egg_array_float_set (array, 4, 100.0);
	value = egg_array_float_get (array, 4);
	if (value == 100.0)
		egg_test_success (test, "got value okay", value);
	else
		egg_test_failed (test, "got wrong value (%f)", value);
	egg_array_float_print (array);

	/************************************************************/
	egg_test_title (test, "make sure we get the correct array sum (2)");
	egg_array_float_set (array, 0, 20.0);
	egg_array_float_set (array, 1, 44.0);
	egg_array_float_set (array, 2, 45.0);
	egg_array_float_set (array, 3, 89.0);
	egg_array_float_set (array, 4, 100.0);
	egg_array_float_set (array, 5, 12.0);
	egg_array_float_set (array, 6, 76.0);
	egg_array_float_set (array, 7, 78.0);
	egg_array_float_set (array, 8, 1.20);
	egg_array_float_set (array, 9, 3.0);
	value = egg_array_float_sum (array);
	if (fabs (value - 468.2) < 0.0001f)
		egg_test_success (test, "got correct sum");
	else
		egg_test_failed (test, "got wrong sum (%f)", value);

	/************************************************************/
	egg_test_title (test, "test convolving with kernel #1");
	egg_array_float_set (array, 0, 0.0);
	egg_array_float_set (array, 1, 0.0);
	egg_array_float_set (array, 2, 0.0);
	egg_array_float_set (array, 3, 0.0);
	egg_array_float_set (array, 4, 100.0);
	egg_array_float_set (array, 5, 0.0);
	egg_array_float_set (array, 6, 0.0);
	egg_array_float_set (array, 7, 0.0);
	egg_array_float_set (array, 8, 0.0);
	egg_array_float_set (array, 9, 0.0);
	result = egg_array_float_convolve (array, kernel);
	if (result->len == 10)
		egg_test_success (test, "got correct size convolve product");
	else
		egg_test_failed (test, "got correct size convolve product (%f)", result->len);
	egg_array_float_print (result);

	/************************************************************/
	egg_test_title (test, "make sure we get the correct array sum of convolve #1");
	value = egg_array_float_sum (result);
	if (fabs(value - 100.0) < 5.0)
		egg_test_success (test, "got correct (enough) sum (%f)", value);
	else
		egg_test_failed (test, "got wrong sum (%f)", value);
	egg_array_float_free (result);

	/************************************************************/
	egg_test_title (test, "test convolving with kernel #2");
	egg_array_float_set (array, 0, 100.0);
	egg_array_float_set (array, 1, 0.0);
	egg_array_float_set (array, 2, 0.0);
	egg_array_float_set (array, 3, 0.0);
	egg_array_float_set (array, 4, 0.0);
	egg_array_float_set (array, 5, 0.0);
	egg_array_float_set (array, 6, 0.0);
	egg_array_float_set (array, 7, 0.0);
	egg_array_float_set (array, 8, 0.0);
	egg_array_float_set (array, 9, 0.0);
	result = egg_array_float_convolve (array, kernel);
	if (result->len == 10)
		egg_test_success (test, "got correct size convolve product");
	else
		egg_test_failed (test, "got correct size convolve product (%f)", result->len);
	egg_array_float_print (array);
	egg_array_float_print (result);

	/************************************************************/
	egg_test_title (test, "make sure we get the correct array sum of convolve #2");
	value = egg_array_float_sum (result);
	if (fabs(value - 100.0) < 10.0)
		egg_test_success (test, "got correct (enough) sum (%f)", value);
	else
		egg_test_failed (test, "got wrong sum (%f)", value);

	egg_array_float_free (result);

	/************************************************************/
	egg_test_title (test, "test convolving with kernel #3");
	egg_array_float_set (array, 0, 0.0);
	egg_array_float_set (array, 1, 0.0);
	egg_array_float_set (array, 2, 0.0);
	egg_array_float_set (array, 3, 0.0);
	egg_array_float_set (array, 4, 0.0);
	egg_array_float_set (array, 5, 0.0);
	egg_array_float_set (array, 6, 0.0);
	egg_array_float_set (array, 7, 0.0);
	egg_array_float_set (array, 8, 0.0);
	egg_array_float_set (array, 9, 100.0);
	result = egg_array_float_convolve (array, kernel);
	if (result->len == 10)
		egg_test_success (test, "got correct size convolve product");
	else
		egg_test_failed (test, "got correct size convolve product (%f)", result->len);
	egg_array_float_print (array);
	egg_array_float_print (result);

	/************************************************************/
	egg_test_title (test, "make sure we get the correct array sum of convolve #3");
	value = egg_array_float_sum (result);
	if (fabs(value - 100.0) < 10.0)
		egg_test_success (test, "got correct (enough) sum (%f)", value);
	else
		egg_test_failed (test, "got wrong sum (%f)", value);
	egg_array_float_free (result);

	/************************************************************/
	egg_test_title (test, "test convolving with kernel #4");
	egg_array_float_set (array, 0, 10.0);
	egg_array_float_set (array, 1, 10.0);
	egg_array_float_set (array, 2, 10.0);
	egg_array_float_set (array, 3, 10.0);
	egg_array_float_set (array, 4, 10.0);
	egg_array_float_set (array, 5, 10.0);
	egg_array_float_set (array, 6, 10.0);
	egg_array_float_set (array, 7, 10.0);
	egg_array_float_set (array, 8, 10.0);
	egg_array_float_set (array, 9, 10.0);
	result = egg_array_float_convolve (array, kernel);
	if (result->len == 10)
		egg_test_success (test, "got correct size convolve product");
	else
		egg_test_failed (test, "got incorrect size convolve product (%f)", result->len);
	egg_array_float_print (array);
	egg_array_float_print (result);

	/************************************************************/
	egg_test_title (test, "make sure we get the correct array sum of convolve #4");
	value = egg_array_float_sum (result);
	if (fabs(value - 100.0) < 1.0)
		egg_test_success (test, "got correct (enough) sum (%f)", value);
	else
		egg_test_failed (test, "got wrong sum (%f)", value);

	/************************************************************/
	egg_test_title (test, "test convolving with kernel #5");
	egg_array_float_set (array, 0, 10.0);
	egg_array_float_set (array, 1, 10.0);
	egg_array_float_set (array, 2, 10.0);
	egg_array_float_set (array, 3, 10.0);
	egg_array_float_set (array, 4, 0.0);
	egg_array_float_set (array, 5, 10.0);
	egg_array_float_set (array, 6, 10.0);
	egg_array_float_set (array, 7, 10.0);
	egg_array_float_set (array, 8, 10.0);
	egg_array_float_set (array, 9, 10.0);
	result = egg_array_float_convolve (array, kernel);
	if (result->len == 10)
		egg_test_success (test, "got correct size convolve product");
	else
		egg_test_failed (test, "got incorrect size convolve product (%f)", result->len);
	egg_array_float_print (array);
	egg_array_float_print (result);

	/************************************************************/
	egg_test_title (test, "make sure we get the correct array sum of convolve #5");
	value = egg_array_float_sum (result);
	if (fabs(value - 90.0) < 1.0)
		egg_test_success (test, "got correct (enough) sum (%f)", value);
	else
		egg_test_failed (test, "got wrong sum (%f)", value);

	/*************** INTEGRATION TEST ************************/
	egg_test_title (test, "integration down");
	egg_array_float_set (array, 0, 0.0);
	egg_array_float_set (array, 1, 1.0);
	egg_array_float_set (array, 2, 2.0);
	egg_array_float_set (array, 3, 3.0);
	egg_array_float_set (array, 4, 4.0);
	egg_array_float_set (array, 5, 5.0);
	egg_array_float_set (array, 6, 6.0);
	egg_array_float_set (array, 7, 7.0);
	egg_array_float_set (array, 8, 8.0);
	egg_array_float_set (array, 9, 9.0);
	size = egg_array_float_compute_integral (array, 0, 4);
	if (size == 0+1+2+3+4)
		egg_test_success (test, "intergrated okay");
	else
		egg_test_failed (test, "did not intergrated okay (%i)", size);
	egg_test_title (test, "integration up");
	size = egg_array_float_compute_integral (array, 5, 9);
	if (size == 5+6+7+8+9)
		egg_test_success (test, "intergrated okay");
	else
		egg_test_failed (test, "did not intergrated okay (%i)", size);
	egg_test_title (test, "integration all");
	size = egg_array_float_compute_integral (array, 0, 9);
	if (size == 0+1+2+3+4+5+6+7+8+9)
		egg_test_success (test, "intergrated okay");
	else
		egg_test_failed (test, "did not intergrated okay (%i)", size);

	/*************** AVERAGE TEST ************************/
	egg_test_title (test, "average");
	egg_array_float_set (array, 0, 0.0);
	egg_array_float_set (array, 1, 1.0);
	egg_array_float_set (array, 2, 2.0);
	egg_array_float_set (array, 3, 3.0);
	egg_array_float_set (array, 4, 4.0);
	egg_array_float_set (array, 5, 5.0);
	egg_array_float_set (array, 6, 6.0);
	egg_array_float_set (array, 7, 7.0);
	egg_array_float_set (array, 8, 8.0);
	egg_array_float_set (array, 9, 9.0);
	value = egg_array_float_get_average (array);
	if (value == 4.5)
		egg_test_success (test, "averaged okay");
	else
		egg_test_failed (test, "did not average okay (%i)", value);

	egg_array_float_free (result);
	egg_array_float_free (array);
	egg_array_float_free (kernel);

	egg_test_end (test);
}

#endif

