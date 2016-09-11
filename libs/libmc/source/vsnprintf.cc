/*
 *    Copyright 2016 Bruno Ribeiro
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include <sys/types.h>
#include <mc/stdarg.h>
#include <mc/string.h>


/**
 * @brief Left-justify within the given field width.
 *
 * Right justification is the default (see width sub-specifier).
 */
#define FLAG_LEFT            (0x10U)

/**
 * @brief Forces to preceed the result with a plus or minus sign (+ or -)
 * even for positive numbers.
 *
 * By default, only negative numbers are preceded with a - sign.
 */
#define FLAG_SIGNAL          (0x04U)

/**
 * @brief If no sign is going to be written, a blank space is inserted before the value.
 */
#define FLAG_SPACE           (0x08U)

/**
 * @brief Format-specific customization.
 */
#define FLAG_CUSTOM          (0x20U)

/**
 * @brief Left-pads the number with zeroes (0) instead of spaces when padding
 * is specified (see width sub-specifier).
 */
#define FLAG_ZERO            (0x01)

#define FLAG_SIGNED          (0x02)

#define FLAG_UPPER           (0x40)

#define is_digit(c) ((c) >= '0' && (c) <= '9')

static const char16_t *LOWER_CHARACTERS = u"0123456789abcdefghijklmnopqrstuvwxyz";
static const char16_t *UPPER_CHARACTERS = u"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

static int stringToNumber(
	const char16_t **text)
{
	int i = 0;
	while (is_digit(**text)) i = i*10 + *((*text)++) - '0';
	return i;
}

static char16_t *numberToString(
	char16_t *output,
	ssize_t value,
	int base,
	int size,
	int precision,
	int type)
{
	char16_t sign = u'\0';
	const char16_t *digits = LOWER_CHARACTERS;

	if (type & FLAG_UPPER) digits = UPPER_CHARACTERS;
	if (type & FLAG_LEFT) type &= ~FLAG_ZERO;
	if (base < 2 || base > 36) return nullptr;

	// detects the filler character
	char16_t filler = (type & FLAG_ZERO) ? u'0' : u' ';
	// checks if we need to print the sign
	if (type & FLAG_SIGNED)
	{
		if (value < 0)
		{
			sign = u'-';
			value = -value;
			size--;
		}
		else if (type & FLAG_SIGNAL)
		{
			sign = u'+';
			size--;
		}
		else if (type & FLAG_SPACE)
		{
			sign = ' ';
			size--;
		}
	}
	// checks if we have some customization
	if (type & FLAG_CUSTOM)
	{
		if (base == 16)
		{
			// we need room for the "0x"
			size -= 2;
		}
		else if (base == 8)
		{
			// we need room for the "0"
			size--;
		}
	}

	// converts the value to string
	int i = 0;
	char16_t tmp[24] = { '0' };
	if (value > 0)
	{
		while (value != 0)
		{
			tmp[i++] = digits[ (size_t) value % base ];
			value = (size_t) value / base;
		}
	}
	else
		++i;

	if (i > precision) precision = i;
	size -= precision;

	// checks if we need to insert leading zeros
	if (!(type & FLAG_ZERO) && !(type & FLAG_LEFT))
		while (size-- > 0) *output++ = ' ';
	// insert the sign
	if (sign != 0) *output++ = sign;
	// checks if we need to insert the custom prefix
	if (type & FLAG_CUSTOM)
	{
		if (base == 8)
		{
			*output++ = '0';
		}
		else if (base == 16)
		{
			*output++ = '0';
			*output++ = digits[33];
		}
	}

	// inserts filler characters
	if ((type & FLAG_LEFT) == 0)
		for ( ; size-- > 0; *output++ = filler );
	while (i < precision--) *output++ = '0';
	while (i-- > 0) *output++ = tmp[i];
	while (size-- > 0) *output++ = ' ';

	return output;
}


int mc_vsnprintf(
	char16_t *output,
	size_t outputSize,
	const char16_t *format,
	va_list args )
{
	if (output == nullptr || outputSize <= 1)
		return 0;

	size_t num;
	int base;
	char16_t *current;

	int precision;        // Min. # of digits for integers; max number of chars for from string
	int qualifier;        // 'h', 'l', or 'L' for integer fields

	for (current = output; *format; format++)
	{
		// copy ordinary characters
		if (*format != '%')
		{
			*current++ = *format;
			continue;
		}
		else
			++format;

		uint32_t flags = 0;
repeat:
		switch (*format++)
		{
			case '-':
				flags |= FLAG_LEFT;
				goto repeat;
			case '+':
				flags |= FLAG_SIGNAL;
				goto repeat;
			case ' ':
				flags |= FLAG_SPACE;
				goto repeat;
			case '#':
				flags |= FLAG_CUSTOM;
				goto repeat;
			case '0':
				flags |= FLAG_ZERO;
				goto repeat;
			default:
				--format;
				break;
		}

		// Get field width
		int32_t width = -1;
		if (is_digit(*format))
		{
			width = stringToNumber(&format);
		}
		else
		if (*format == '*')
		{
			format++;
			width = va_arg(args, int);
			if (width < 0)
			{
				width = -width;
				flags |= FLAG_LEFT;
			}
		}

		// Get the precision
		precision = -1;
		if (*format == '.')
		{
			++format;
			if (is_digit(*format))
			{
				precision = stringToNumber(&format);
			}
			else
			if (*format == '*')
			{
				++format;
				precision = va_arg(args, int);
			}
			if (precision < 0) precision = 0;
		}

		// Get the conversion qualifier
		qualifier = -1;
		if (*format == 'h' || *format == 'l' || *format == 'L')
		{
			qualifier = *format;
			format++;
		}

		// Default base
		base = 10;

		switch (*format)
		{
			case 'c':
				if (flags & FLAG_LEFT)
					for (; --width > 0; *current++ = ' ');
				*current++ = (char) va_arg(args, int);
				while (--width > 0) *current++ = ' ';
				continue;

			case 's':
			{
				const char16_t *text = va_arg(args, char16_t *);
				if (!text)
					text = u"<NULL>";

				int32_t length = (int32_t) mc_strnlen(text, (size_t) precision);

				if ((flags & FLAG_LEFT) == 0)
					while (length < width--) *current++ = ' ';
				for (int32_t i = 0; i < length; ++i) *current++ = *text++;
				while (length < width--) *current++ = ' ';
				continue;
			}

			case 'p':
				if (width == -1)
				{
					width = 2 * sizeof(void *);
					flags |= FLAG_ZERO;
				}
				current = numberToString(current, (long) va_arg(args, void *), 16, width, precision, flags);
				continue;

			case 'n':
				if (qualifier == 'l')
				{
					long *ip = va_arg(args, long *);
					*ip = (current - output);
				}
				else
				{
					int *ip = va_arg(args, int *);
					*ip = (current - output);
				}
				continue;

			// Integer number formats - set up the flags and "break"
			case 'o':
				base = 8;
				break;

			case 'X':
				flags |= FLAG_UPPER;
				// passthrough

			case 'x':
				base = 16;
				break;

			case 'd':
			case 'i':
				flags |= FLAG_SIGNAL;

			case 'u':
				break;

			default:
				if (*format != '%') *current++ = '%';
				if (*format)
				{
					*current++ = *format;
				}
				else
				{
					--format;
				}
				continue;
		}

		if (qualifier == 'l')
		{
			num = va_arg(args, unsigned long);
		}
		else
		if (qualifier == 'h')
		{
			if (flags & FLAG_SIGNAL)
			{
				num = va_arg(args, short);
			}
			else
			{
				num = va_arg(args, unsigned short);
			}
		} else if (flags & FLAG_SIGNAL)
		{
			num = va_arg(args, int);
		}
		else
		{
			num = va_arg(args, unsigned int);
		}

		current = numberToString(current, num, base, width, precision, flags);
	}

	*current = '\0';
	return current - output;
}


int mc_snprintf(
	char16_t *buffer,
	size_t size,
	const char16_t *format,
	... )
{
	va_list args;
	int n;

	va_start(args, format);
	n = mc_vsnprintf(buffer, size, format, args);
	va_end(args);

	return n;
}