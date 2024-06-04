﻿#include "format.h"
#include <base/system.h>

void fmt_init_handler_func(HandlerFmtCallbackFunc* pCallback, void* pData)
{
	struct_handler_fmt::init(pCallback, pData);
}

void fmt_set_flags(int flags)
{
	struct_handler_fmt::use_flags(flags);
}

std::string struct_format_implement::impl_end_result(const description&, const std::string& Text, std::deque<std::string>& vPack)
{
	// initialize variables
	enum
	{
		truncation_nope,
		truncation_full,
		truncation_custom
	};
	std::string Result {};
	size_t argumentPosition = 0;
	bool argumentProcessing = false;
	const int length = (int)Text.length();
	int truncationValue {};
	int truncationType {};
	char truncationAfterChar { '\0' };

	// use instead const char* std::string
	for(int i = 0; i < length; ++i)
	{
		const char iterChar = Text[i];

		// start argument
		if(iterChar == '{')
		{
			argumentProcessing = true;
			continue;
		}

		// argument started
		if(!argumentProcessing)
		{
			Result += iterChar;
			continue;
		}

		// truncation
		if(iterChar == '~')
		{
			std::string truncationString;
			for(int j = i + 1; Text[j] != '}' && j != length; ++j)
			{
				// align custom value from fmt
				if(Text[j] == '%' && argumentPosition < vPack.size())
				{
					truncationString = vPack[argumentPosition++];
					continue;
				}

				// align custom value from text
				if(Text[j] >= '0' && Text[j] <= '9')
				{
					truncationString += Text[j];
					continue;
				}

				truncationAfterChar = Text[j];
			}

			truncationValue = truncationString.empty() ? 0 : str_toint(truncationString.c_str());
			truncationType = (truncationAfterChar == '\0') ? truncation_full : truncation_custom;
			continue;
		}

		// argument ending
		if(iterChar == '}')
		{
			argumentProcessing = false;
			if(argumentPosition >= vPack.size())
				continue;

			// initialize variables
			std::string arg = vPack[argumentPosition++];

			// truncation prepare value
			if(truncationType != truncation_nope)
			{
				if(truncationType == truncation_full && arg.size() > (size_t)truncationValue)
				{
					arg.resize(truncationValue);
				}
				else if(truncationType == truncation_custom && arg.find(truncationAfterChar) != std::string::npos)
				{
					const size_t dotPos = arg.find(truncationAfterChar) + 1;
					arg = arg.substr(0, dotPos + truncationValue);
				}

				truncationValue = 0;
				truncationAfterChar = '\0';
				truncationType = truncation_nope;
			}

			Result += arg;
		}
	}

	// result string
	return Result;
}