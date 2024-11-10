﻿/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_CORE_TEMPLATE_UTILS_H
#define GAME_SERVER_CORE_TEMPLATE_UTILS_H

using ByteArray = std::basic_string<std::byte>;

/**
 * @brief Convert JSON value to BigInt.
 *
 * This function overload nlohmann json checks if the JSON value is either a string or a number.
 * If the value is a string, it directly converts it to BigInt.
 */
inline void from_json(const nlohmann::json& j, BigInt& value)
{
	if(j.is_string())
	{
		value = BigInt(j.get<std::string>());
	}
	else if(j.is_number())
	{
		value = BigInt(j.dump());
	}
	else
	{
		throw std::invalid_argument("Unsupported JSON type for BigInt");
	}
}
inline void to_json(nlohmann::json& j, const BigInt& value)
{
	j = value.to_string();
}


/**
 * @namespace mystd
 * @brief A set of utilities for advanced work with containers, concepts and configuration.
 *
 * The `mystd` namespace contains functions and classes for working with containers,
 * smart pointers and configurations, as well as utilities for parsing strings and freeing resources.
 */
namespace mystd
{
	namespace detail
	{
		// specialization parsing value
		namespace optparse
		{
			template<typename T> inline std::optional<T> Value(const std::string& str) { return std::nullopt; };
			template<> inline std::optional<bool> Value<bool>(const std::string& str) { return str_toint(str.c_str()) > 0; }
			template<> inline std::optional<float> Value<float>(const std::string& str) { return str_tofloat(str.c_str()); }
			template<> inline std::optional<double> Value<double>(const std::string& str) { return str_tofloat(str.c_str()); }
			template<> inline std::optional<int> Value<int>(const std::string& str) { return str_toint(str.c_str()); }
			template<> inline std::optional<std::string> Value<std::string>(const std::string& str) { return str; }
			template<> inline std::optional<BigInt> Value<BigInt>(const std::string& str) { return BigInt(str); }
			template<> inline std::optional<vec2> Value<vec2>(const std::string& str)
			{
				const auto pos = str.find(' ');
				if(pos == std::string::npos)
				{
					const auto errorStr = "invalid vec2 parse format: " + str;
					dbg_assert(false, errorStr.c_str());
					return std::nullopt;
				}

				const std::string_view xStr = str.substr(0, pos);
				const std::string_view yStr = str.substr(pos + 1);
				const float x = str_tofloat(xStr.data());
				const float y = str_tofloat(yStr.data());
				return vec2(x, y);
			}
		}

		// сoncepts
		template<typename T>
		concept is_has_clear_function = requires(T & c)
		{
			{ c.clear() } -> std::same_as<void>;
		};

		template<typename T>
		concept is_smart_pointer = requires(T & c)
		{
			{ c.get() } -> std::convertible_to<typename T::element_type*>;
			{ c.reset() } noexcept -> std::same_as<void>;
		};

		template <typename T>
		concept is_container = requires(T & c)
		{
			typename T::value_type;
			typename T::iterator;
			{ c.begin() } -> std::convertible_to<typename T::iterator>;
			{ c.end() } -> std::convertible_to<typename T::iterator>;
		};

		template<typename T>
		concept is_map_container = requires(T & c)
		{
			typename T::key_type;
			typename T::mapped_type;
				requires is_container<T>;
				requires std::same_as<typename T::value_type, std::pair<const typename T::key_type, typename T::mapped_type>>;
		};
	}

	// function to clear a container
	template<detail::is_container T>
	void freeContainer(T& container)
	{
		static_assert(detail::is_has_clear_function<T>, "One or more types do not have clear() function");

		if constexpr(detail::is_map_container<T>)
		{
			if constexpr(detail::is_container<typename T::mapped_type>)
				std::ranges::for_each(container, [](auto& element) { freeContainer(element.second); });
			else if constexpr(std::is_pointer_v<typename T::mapped_type> && !detail::is_smart_pointer<typename T::mapped_type>)
				std::ranges::for_each(container, [](auto& element) { delete element.second; });
		}
		else
		{
			if constexpr(detail::is_container<typename T::value_type>)
				std::ranges::for_each(container, [](auto& element) { freeContainer(element); });
			else if constexpr(std::is_pointer_v<typename T::value_type> && !detail::is_smart_pointer<typename T::value_type>)
				std::ranges::for_each(container, [](auto& element) { delete element; });
		}
		container.clear();
	}
	template<detail::is_container... Containers>
	void freeContainer(Containers&... args) { (freeContainer(args), ...); }

	template<typename... Args>
	bool loadSettings(const std::string& prefix, const std::string& line, Args*... values)
	{
		return loadSettings(prefix, std::vector{line}, values...);
	}

	template<typename... Args>
	bool loadSettings(const std::string& prefix, const std::vector<std::string>& lines, Args*... values)
	{
		size_t CurrentPos = 0;
		bool Success = true;

		// helper function to load a single value based on prefix
		auto loadSingleValue = [&CurrentPos, &lines](const std::string& fullPrefix) -> std::optional<std::string>
		{
			auto it = std::ranges::find_if(lines, [&CurrentPos, &fullPrefix](const std::string& s)
			{
				return s.starts_with(fullPrefix);
			});

			if(it != lines.end())
			{
				auto valueStr = it->substr(fullPrefix.size() + CurrentPos);
				valueStr.erase(valueStr.begin(), std::find_if_not(valueStr.begin(), valueStr.end(), [](unsigned char c)
				{
					return std::isspace(c);
				}));

				// is by quotes parser
				if(valueStr.starts_with('"'))
				{
					const auto end = valueStr.find('\"', 1);
					if(end != std::string::npos)
					{
						CurrentPos += end + 1;
						return valueStr.substr(1, end - 1);
					}

					return std::nullopt;
				}

				// default parser
				const auto end = valueStr.find_first_of(" \t\n\r");
				if(end != std::string::npos)
				{
					CurrentPos += end;
					return valueStr.substr(0, end);
				}

				return valueStr;
			}

			return std::nullopt;
		};


		// lambda to load and parse a single value
		auto loadAndParse = [&](auto* value)
		{
			using ValueType = std::remove_pointer_t<decltype(value)>;
			const auto fullPrefix = prefix + " ";

			if(auto valueStrOpt = loadSingleValue(fullPrefix); valueStrOpt.has_value())
			{
				if(auto parsedValue = detail::optparse::Value<ValueType>(valueStrOpt.value()); parsedValue.has_value())
					*value = parsedValue.value();
				else
					Success = false;
			}
			else
				Success = false;
		};

		// use fold expression
		(loadAndParse(values), ...);
		return Success;
	}

	template <typename T> requires std::is_arithmetic_v<T>
	void process_bigint_in_chunks(BigInt total, std::invocable<T> auto processChunk)
	{
		constexpr T maxChunkSize = std::numeric_limits<T>::max();
		while(total > 0)
		{
			T currentChunk = maxChunkSize;
			if(total < maxChunkSize)
			{
				// always true
				currentChunk = total.to_int();
			}
			processChunk(currentChunk);
			total -= currentChunk;
		}
	}

	/**
	 * @namespace string
	 * @brief Utilities for working with text strings.
	 *
	 * The `string` namespace contains functions for working with strings, which can be used
	 * for various purposes of formatting, displaying, or manipulating text.
	 */
	namespace string
	{
		inline std::string progressBar(uint64_t maxValue, uint64_t currentValue, int totalSteps, const std::string& Fillsymbols, const std::string& EmptySymbols)
		{
			std::string resutStr;
			const auto numFilled = currentValue / totalSteps;
			const auto numEmpty = maxValue / totalSteps - numFilled;
			resutStr.reserve(numFilled + numEmpty);

			for(int i = 0; i < numFilled; i++)
			{
				resutStr += Fillsymbols;
			}

			for(int i = 0; i < numEmpty; i++)
			{
				resutStr += EmptySymbols;
			}

			return resutStr;
		}
	}


	/**
	 * @namespace json
	 * @brief Utilities for working with JSON data using the nlohmann::json library.
	 *
	 * The `json` namespace provides functions to parse strings in JSON format
	 * and perform actions on the resulting data via callback functions.
	 */
	namespace json
	{
		inline void parse(const std::string& Data, const std::function<void(nlohmann::json& pJson)>& pFuncCallback)
		{
			if(!Data.empty())
			{
				try
				{
					nlohmann::json JsonData = nlohmann::json::parse(Data);
					pFuncCallback(JsonData);
				}
				catch(nlohmann::json::exception& s)
				{
					dbg_assert(false, fmt_default("[json parse] Invalid json: {}", s.what()).c_str());
				}
			}
		}
	}


	/**
	 * @namespace file
	 * @brief Utilities for working with the file system: reading, writing and deleting files.
	 *
	 * The `file` namespace contains functions for performing file operations,
	 * such as load, save, and delete. These functions return the result of the operation
	 * as a `Result` enumeration.
	 */
	namespace file
	{
		enum result : int
		{
			ERROR_FILE,
			SUCCESSFUL,
		};

		inline result load(const char* pFile, ByteArray* pData)
		{
			IOHANDLE File = io_open(pFile, IOFLAG_READ);
			if(!File)
				return ERROR_FILE;

			pData->resize((unsigned)io_length(File));
			io_read(File, pData->data(), (unsigned)pData->size());
			io_close(File);
			return SUCCESSFUL;
		}

		inline result remove(const char* pFile)
		{
			int Result = fs_remove(pFile);
			return Result == 0 ? SUCCESSFUL : ERROR_FILE;
		}

		inline result save(const char* pFile, const void* pData, unsigned size)
		{
			// delete old file
			remove(pFile);

			IOHANDLE File = io_open(pFile, IOFLAG_WRITE);
			if(!File)
				return ERROR_FILE;

			io_write(File, pData, size);
			io_close(File);
			return SUCCESSFUL;
		}
	}


	/**
	 * @namespace aesthetic
	 * @brief A set of utilities for creating aesthetically pleasing text strings with decorative borders and symbols.
	 *
	 * The `aesthetic` namespace provides functions for creating text elements,
	 * such as borders, symbols, lines, and quotes, which can be used for
	 * output design in applications.
	 */
	namespace aesthetic
	{
		// Example: ───※ ·· ※───
		inline std::string boardPillar(const std::string_view& text, int iter = 5)
		{
			std::string result;
			result.reserve(iter * 2 + text.size() + 10);

			for(int i = 0; i < iter; ++i)
				result += "\u2500";
			result += "\u203B \u00B7 ";
			result += text;
			result += " \u00B7 \u203B";
			for(int i = 0; i < iter; ++i)
				result += "\u2500";

			return std::move(result);
		}

		// Example: ✯¸.•*•✿✿•*•.¸✯
		inline std::string boardFlower(const std::string_view& text)
		{
			std::string result;
			result.reserve(text.size() + 10);

			result += "\u272F\u00B8.\u2022*\u2022\u273F";
			result += text;
			result += "\u273F\u2022*\u2022.\u00B8\u272F";

			return std::move(result);
		}

		// Example: ──⇌ • • ⇋──
		inline std::string boardConfident(const std::string_view& text, int iter = 5)
		{
			std::string result;
			result.reserve(iter * 2 + text.size() + 10);

			for(int i = 0; i < iter; ++i)
				result += "\u2500";
			result += " \u21CC \u2022 ";
			result += text;
			result += " \u2022 \u21CB ";
			for(int i = 0; i < iter; ++i)
				result += "\u2500";

			return std::move(result);
		}

		/* LINES */
		// Example: ︵‿︵‿
		template <int iter>
		inline std::string lineWaves()
		{
			std::string result;
			result.reserve(iter * 2);

			for(int i = 0; i < iter; ++i)
				result += "\uFE35\u203F";

			return result;
		}

		/* WRAP LINES */
		// Example:  ────⇌ • • ⇋────
		inline std::string wrapLineConfident(int iter)
		{
			return boardConfident("", iter);
		}

		// Example: ───※ ·· ※───
		inline std::string wrapLinePillar(int iter)
		{
			return boardPillar("", iter);
		}

		/* QUOTES */
		// Example: -ˏˋ Text here ˊˎ
		inline std::string quoteDefault(const std::string_view& text)
		{
			std::string result;
			result.reserve(text.size() + 10);

			result += "-\u02CF\u02CB";
			result += text;
			result += "\u02CA\u02CE";

			return result;
		}

		/* SYMBOL SMILIES */
		// Example: ᴄᴏᴍᴘʟᴇᴛᴇ!
		inline std::string symbolsComplete()
		{
			return "\u1D04\u1D0F\u1D0D\u1D18\u029F\u1D07\u1D1B\u1D07!";
		}
	}

	class CConfigurable
	{
		using ConfigVariant = std::variant<int, float, vec2, std::string, std::vector<vec2>>;
		ska::flat_hash_map<std::string, ConfigVariant> m_umConfig;

	public:
		void SetConfig(const std::string& key, const ConfigVariant& value) { m_umConfig[key] = value; }
		bool HasConfig(const std::string& key) const { return m_umConfig.find(key) != m_umConfig.end(); }

		template<typename T>
		T GetConfig(const std::string& key, const T& defaultValue) const
		{
			if(const auto it = m_umConfig.find(key); it != m_umConfig.end())
			{
				if(std::holds_alternative<T>(it->second))
					return std::get<T>(it->second);

				dbg_assert(false, fmt_default("Type mismatch for key: {}", key).c_str());
			}
			return defaultValue;
		}

		template<typename T>
		T& GetRefConfig(const std::string& key, const T& defaultValue)
		{
			auto& variant = m_umConfig[key];
			if(!std::holds_alternative<T>(variant))
			{
				dbg_assert(false, fmt_default("Type mismatch for key: {}\n", key).c_str());
				variant = defaultValue;
			}

			return std::get<T>(variant);
		}

		template<typename T>
		bool TryGetConfig(const std::string& key, T& outValue) const
		{
			if(const auto it = m_umConfig.find(key); it != m_umConfig.end())
			{
				if(std::holds_alternative<T>(it->second))
				{
					outValue = std::get<T>(it->second);
					return true;
				}
			}
			return false;
		}

		void RemoveConfig(const std::string& key)
		{
			m_umConfig.erase(key);
		}
	};
}

#endif
