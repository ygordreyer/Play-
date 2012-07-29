#include "Iop_Stdio.h"
#include <boost/lexical_cast.hpp>
#include "lexical_cast_ex.h"
#include "../Log.h"

#define LOG_NAME			"iop_stdio"

#define FUNCTION_PRINTF		"printf"

using namespace Iop;

CStdio::CStdio(uint8* ram) 
: m_ram(ram)
{

}

CStdio::~CStdio()
{

}

std::string CStdio::GetId() const
{
	return "stdio";
}

std::string CStdio::GetFunctionName(unsigned int functionId) const
{
	switch(functionId)
	{
	case 4:
		return FUNCTION_PRINTF;
		break;
	default:
		return "unknown";
		break;
	}
}

void CStdio::Invoke(CMIPS& context, unsigned int functionId)
{
	switch(functionId)
	{
	case 4:
		__printf(context);
		break;
	default:
		CLog::GetInstance().Print(LOG_NAME, "Unknown function (%d) called. PC = (%0.8X).", 
			functionId, context.m_State.nPC);
		break;
	}
}

std::string CStdio::PrintFormatted(CArgumentIterator& args)
{
	std::string output;
	const char* format = reinterpret_cast<const char*>(&m_ram[args.GetNext()]);
	while(*format != 0)
	{
		char character = *(format++);
		if(character == '%')
		{
			bool paramDone = false;
			bool inPrecision = false;
			char fillChar = ' ';
			std::string precision;
			while(!paramDone && *format != 0) 
			{
				char type = *(format++);
				if(type == 's')
				{
					const char* text = reinterpret_cast<const char*>(&m_ram[args.GetNext()]);
					output += text;
					paramDone = true;
				}
				else if(type == 'c')
				{
					char character = static_cast<char>(args.GetNext());
					output += character;
					paramDone = true;
				}
				else if(type == 'd' || type == 'i')
				{
					int number = args.GetNext();
					unsigned int precisionValue = precision.length() ? boost::lexical_cast<unsigned int>(precision) : 1;
					output += lexical_cast_int<std::string>(number, precisionValue, fillChar);
					paramDone = true;
				}
				else if(type == 'u')
				{
					unsigned int number = args.GetNext();
					unsigned int precisionValue = precision.length() ? boost::lexical_cast<unsigned int>(precision) : 1;
					output += lexical_cast_uint<std::string>(number, precisionValue);
					paramDone = true;
				}
				else if(type == 'x' || type == 'X' || type == 'p')
				{
					uint32 number = args.GetNext();
					unsigned int precisionValue = precision.length() ? boost::lexical_cast<unsigned int>(precision) : 0;
					output += lexical_cast_hex<std::string>(number, precisionValue);
					paramDone = true;
				}
				else if(type == 'l')
				{
					//Length specifier, don't bother about it.
				}
				else if(type == '.')
				{
					inPrecision = true;
				}
				else
				{
					assert(isdigit(type));
					if(inPrecision)
					{
						precision += type;
					}
					else
					{
						fillChar = type;
						inPrecision = true;
					}
				}
			}
		}
		else
		{
			output += character;
		}
	}
	return output;
}

void CStdio::__printf(CMIPS& context)
{
	CArgumentIterator args(context);
	std::string output = PrintFormatted(args);
	printf("%s", output.c_str());
}
