#ifndef HASH_UTILS_HPP
#define HASH_UTILS_HPP
#include <openssl/sha.h>
#include <iomanip>
#include <sstream>
#include <string>

std::string calculate_sha256(const std::string& content);
#endif