#include <scorum/wallet/utils.hpp>

#include <graphene/utilities/key_conversion.hpp>
#include <graphene/utilities/words.hpp>

#define BRAIN_KEY_WORD_COUNT 16

namespace scorum {
namespace wallet {

std::string pubkey_to_shorthash(const sp::public_key_type& key)
{
    uint32_t x = fc::sha256::hash(key)._hash[0];
    static const char hd[] = "0123456789abcdef";
    std::string result;

    result += hd[(x >> 0x1c) & 0x0f];
    result += hd[(x >> 0x18) & 0x0f];
    result += hd[(x >> 0x14) & 0x0f];
    result += hd[(x >> 0x10) & 0x0f];
    result += hd[(x >> 0x0c) & 0x0f];
    result += hd[(x >> 0x08) & 0x0f];
    result += hd[(x >> 0x04) & 0x0f];
    result += hd[(x)&0x0f];

    return result;
}

std::string normalize_brain_key(const std::string& s)
{
    size_t i = 0, n = s.length();
    std::string result;
    char c;
    result.reserve(n);

    bool preceded_by_whitespace = false;
    bool non_empty = false;
    while (i < n)
    {
        c = s[i++];
        switch (c)
        {
        case ' ':
        case '\t':
        case '\r':
        case '\n':
        case '\v':
        case '\f':
            preceded_by_whitespace = true;
            continue;

        case 'a':
            c = 'A';
            break;
        case 'b':
            c = 'B';
            break;
        case 'c':
            c = 'C';
            break;
        case 'd':
            c = 'D';
            break;
        case 'e':
            c = 'E';
            break;
        case 'f':
            c = 'F';
            break;
        case 'g':
            c = 'G';
            break;
        case 'h':
            c = 'H';
            break;
        case 'i':
            c = 'I';
            break;
        case 'j':
            c = 'J';
            break;
        case 'k':
            c = 'K';
            break;
        case 'l':
            c = 'L';
            break;
        case 'm':
            c = 'M';
            break;
        case 'n':
            c = 'N';
            break;
        case 'o':
            c = 'O';
            break;
        case 'p':
            c = 'P';
            break;
        case 'q':
            c = 'Q';
            break;
        case 'r':
            c = 'R';
            break;
        case 's':
            c = 'S';
            break;
        case 't':
            c = 'T';
            break;
        case 'u':
            c = 'U';
            break;
        case 'v':
            c = 'V';
            break;
        case 'w':
            c = 'W';
            break;
        case 'x':
            c = 'X';
            break;
        case 'y':
            c = 'Y';
            break;
        case 'z':
            c = 'Z';
            break;

        default:
            break;
        }
        if (preceded_by_whitespace && non_empty)
            result.push_back(' ');
        result.push_back(c);
        preceded_by_whitespace = false;
        non_empty = true;
    }
    return result;
}

fc::ecc::private_key derive_private_key(const std::string& prefix_string, int sequence_number)
{
    std::string sequence_string = std::to_string(sequence_number);
    fc::sha512 h = fc::sha512::hash(prefix_string + " " + sequence_string);
    fc::ecc::private_key derived_key = fc::ecc::private_key::regenerate(fc::sha256::hash(h));
    return derived_key;
}

brain_key_info suggest_brain_key()
{
    brain_key_info result;
    // create a private key for secure entropy
    fc::sha256 sha_entropy1 = fc::ecc::private_key::generate().get_secret();
    fc::sha256 sha_entropy2 = fc::ecc::private_key::generate().get_secret();
    fc::bigint entropy1(sha_entropy1.data(), sha_entropy1.data_size());
    fc::bigint entropy2(sha_entropy2.data(), sha_entropy2.data_size());
    fc::bigint entropy(entropy1);
    entropy <<= 8 * sha_entropy1.data_size();
    entropy += entropy2;
    std::string brain_key = "";

    for (int i = 0; i < BRAIN_KEY_WORD_COUNT; i++)
    {
        fc::bigint choice = entropy % graphene::words::word_list_size;
        entropy /= graphene::words::word_list_size;
        if (i > 0)
            brain_key += " ";
        brain_key += graphene::words::word_list[choice.to_int64()];
    }

    brain_key = normalize_brain_key(brain_key);
    fc::ecc::private_key priv_key = derive_private_key(brain_key, 0);
    result.brain_priv_key = brain_key;
    result.wif_priv_key = graphene::utilities::key_to_wif(priv_key);
    result.pub_key = priv_key.get_public_key();
    return result;
}

} // namespace scorum
} // namespace wallet
