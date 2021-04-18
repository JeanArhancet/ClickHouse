#include <Functions/FunctionsTextClassification.h>
#include <Common/StringUtils/StringUtils.h>
#include <Common/FrequencyHolder.h>
#include <Functions/FunctionFactory.h>
#include <Common/UTF8Helpers.h>
#include <IO/ReadBufferFromString.h>
#include <IO/ReadHelpers.h>

#include <unordered_map>
namespace DB
{


struct TonalityClassificationImpl
{

    using ResultType = String;


    static String get_tonality(const Float64 & tonality_level)
    {
        if (tonality_level < 0.25) { return "NEG"; }
        if (tonality_level > 0.5) { return "POS"; }
        return "NEUT";
    } 
    
    static void constant(String data, String & res)
    {
        static std::unordered_map<String, Float64> emotional_dict = FrequencyHolder::getInstance().getEmotionalDict();

        Float64 weight = 0;
        Float64 count_words = 0;

        String answer;
        String word;

        for (size_t i = 0; i < data.size();)
        {
            if (!isASCII(data[i]))
            {
                word.push_back(data[i]);
                ++i;

                while ((i < data.size()) && (!isASCII(data[i]))) {
                    word.push_back(data[i]);
                    ++i;
                }
                if (emotional_dict.find(word) != emotional_dict.cend())
                {
                    count_words += 1;
                    weight += emotional_dict[word];
                }
                word = "";
            } 
            else
            {
                ++i;
            }
        }

        Float64 total_tonality = weight / count_words;
        res += get_tonality(total_tonality);
    }


    static void vector(
        const ColumnString::Chars & data,
        const ColumnString::Offsets & offsets,
        ColumnString::Chars & res_data,
        ColumnString::Offsets & res_offsets)
    {
        static std::unordered_map<String, Float64> emotional_dict = FrequencyHolder::getInstance().getEmotionalDict();

        res_data.reserve(1024);
        res_offsets.resize(offsets.size());

        size_t prev_offset = 0;
        size_t res_offset = 0;

        for (size_t i = 0; i < offsets.size(); ++i)
        {
            const char * haystack = reinterpret_cast<const char *>(&data[prev_offset]);
            String str = haystack;

            String buf;

            Float64 weight = 0;
            Float64 count_words = 0;


            String answer;
            String word;

            for (size_t ind = 0; ind < str.size();)
            {
                if (!isASCII(str[ind]))
                {
                    word.push_back(str[ind]);
                    ++ind;

                    while ((ind < str.size()) && (!isASCII(str[ind]))) {
                        word.push_back(str[ind]);
                        ++ind;
                    }
                    if (emotional_dict.find(word) != emotional_dict.cend())
                    {
                        count_words += 1;
                        weight += emotional_dict[word];
                    }
                    word = "";
                }
                else
                {
                    ++ind;
                }
            }

            Float64 total_tonality = weight / count_words;
            buf = get_tonality(total_tonality);

            const auto ans = buf.c_str();
            size_t cur_offset = offsets[i];

            res_data.resize(res_offset + strlen(ans) + 1);
            memcpy(&res_data[res_offset], ans, strlen(ans));
            res_offset += strlen(ans);

            res_data[res_offset] = 0;
            ++res_offset;

            res_offsets[i] = res_offset;
            prev_offset = cur_offset;
        }
    }


};

struct NameGetTonality
{
    static constexpr auto name = "getTonality";
};


using FunctionGetTonality = FunctionsTextClassification<TonalityClassificationImpl, NameGetTonality>;

void registerFunctionsTonalityClassification(FunctionFactory & factory)
{
    factory.registerFunction<FunctionGetTonality>();
}

}
