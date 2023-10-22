/**
 * @file core.hpp
 * @author Okano Tomoyuki (tomoyuki.okano@tsuneishi.com)
 * @brief 
 * @version 0.1
 * @date 2023-09-29 Created by Okano Tomoyuki.
 * @date 2023-10-02 Updated by Okano Tomoyuki.
 * @n 1.Add copy function
 * @n 2.Add show function,
 * @n 3.Add DEBUG_PRINT Macro
 * @date 2023-10-03 Updated by Okano Tomoyuki.
 * @n 1.Add algorithm header(std::max, std::min included in this header)
 * @n 2.Add DEBUG_CAUTION,DEBUG_WARNING,DEBUG_CRITICAL Macro
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef _UTILITY_CORE_HPP_
#define _UTILITY_CORE_HPP_

#include <iostream>
#include <numeric>
#include <vector>
#include <cmath>
#include <string>
#include <sstream>
#include <typeinfo>
#include <algorithm>

#ifdef __unix__
    #include <cxxabi.h>
#endif

#define DEBUG_MODE 4 /** set one of {0 , 1, 2, 3, 4} */

#ifdef DEBUG_MODE
    #ifdef __unix__ // linux
        #define DEBUG_PRINT(var)    if(DEBUG_MODE>3){do{int status; std::cout <<         "variable:" << #var << " (type:" << abi::__cxa_demangle(typeid(var).name(), 0, 0, &status) << ")\n";Utility::show(var);}while(0);}
        #define DEBUG_CAUTION(var)  if(DEBUG_MODE>2){do{int status; std::cout << "\033[32mvariable:" << #var << " (type:" << abi::__cxa_demangle(typeid(var).name(), 0, 0, &status) << ")\n";Utility::show(var);std::cout<<"\033[m";}while(0);}
        #define DEBUG_WARNING(var)  if(DEBUG_MODE>1){do{int status; std::cout << "\033[33mvariable:" << #var << " (type:" << abi::__cxa_demangle(typeid(var).name(), 0, 0, &status) << ")\n";Utility::show(var);std::cout<<"\033[m";}while(0);}
        #define DEBUG_CRITICAL(var) if(DEBUG_MODE>0){do{int status; std::cout << "\033[31mvariable:" << #var << " (type:" << abi::__cxa_demangle(typeid(var).name(), 0, 0, &status) << ")\n";Utility::show(var);std::cout<<"\033[m";}while(0);}
    #else           // windows
        #define DEBUG_PRINT(var)    if(DEBUG_MODE>3){do{std::cout << "variable:" << #var << " (type:" << typeid(var).name() << ")\n";Utility::show(var);}while(0);}
        #define DEBUG_CAUTION(var)  if(DEBUG_MODE>2){do{std::cout << "\033[32mvariable:" << #var << " (type:" << typeid(var).name() << ")\n";Utility::show(var);std::cout<<"\033[m";}while(0);}
        #define DEBUG_WARNING(var)  if(DEBUG_MODE>1){do{std::cout << "\033[33mvariable:" << #var << " (type:" << typeid(var).name() << ")\n";Utility::show(var);std::cout<<"\033[m";}while(0);}
        #define DEBUG_CRITICAL(var) if(DEBUG_MODE>0){do{std::cout << "\033[31mvariable:" << #var << " (type:" << typeid(var).name() << ")\n";Utility::show(var);std::cout<<"\033[m";}while(0);}
    #endif
#endif

namespace Utility
{

double radians(const double& value) noexcept
{
    return value * M_PI / 180.;
}

double degrees(const double& value) noexcept
{
    return value * 180. / M_PI;
}

double meter_per_seconds(const double& value) noexcept
{
    return value * 1852. / 3600.;
}

double knots(const double& value) noexcept
{
    return value * 3600. / 1852.;
}

template<typename T>
T limit(const T& value, const T& limit) noexcept
{
    return std::max(std::min(value, limit), -limit);
}

template<typename T>
T limit(const T& value, const T& lower_limit, const T& upper_limit) noexcept
{
    return std::max(std::min(value, upper_limit), lower_limit);
}

double adjust_pi(const double& value) noexcept
{
    double result = value;
    while(result>M_PI)
        result -= 2*M_PI;
    while(result<-M_PI)
        result += 2*M_PI;
    return result;
}

double adjust_180(const double& value) noexcept
{
    double result = value;
    while(result>180.)
        result -= 360.;
    while(result<-180.)
        result += 360.;
    return result;
}

template<typename T>
std::string concat(const std::vector<T>& origin, const char& separator=',') noexcept
{
    std::string result;
    for (const auto& str : origin)
        result += std::to_string(str) + separator;
    result.pop_back(); // erase separator character placed in end.
    return result;
}

std::vector<std::string> split(const std::string& origin, const char& separator=',') noexcept
{
    std::vector<std::string> result;
    std::string element;
    std::istringstream iss(origin);
    while(std::getline(iss, element, separator))
    {
        result.push_back(element);
    }

    return result;
}

std::vector<int> range(const int& end) noexcept
{
    std::vector<int> result(end, 0);
    std::iota(result.begin(), result.end(), 0);
    return result;
}

std::vector<int> range(const int& start, const int& end, const int& interval=1) noexcept
{
    std::vector<int> result((std::ceil(static_cast<float>(end-start)/interval)), 0);
    std::iota(result.begin(), result.end(), 0);
    for (auto& i : result)
    {
        i = start + interval * i;
    }
    return result;
}

template<size_t ROW, size_t COL, typename T>
bool copy(T (&source)[ROW][COL], std::vector<std::vector<T>>& target, const size_t& row_size=ROW, const size_t& column_size=COL)
{
    if(!(row_size>0 && column_size>0 && row_size<=ROW && column_size<=COL))
        return false;
    
    target.resize(row_size);
    for (auto& row : target)
        row.resize(column_size);

    for (const auto& i : range(row_size))
        for (const auto& j : range(column_size))
            target[i][j] = source[i][j];

    return true;
}

template<size_t ROW, size_t COL, typename T>
bool copy(std::vector<std::vector<T>>& source, T (&target)[ROW][COL], const size_t& row_size=ROW, const size_t& column_size=COL)
{
    if(!(row_size>0 && column_size>0 && row_size<=ROW && column_size<=COL))
        return false;
    
    target.resize(row_size);
    for (auto& row : target)
        row.resize(column_size);

    for (const auto& i : range(row_size))
        for (const auto& j : range(column_size))
            target[i][j] = source[i][j];

    return true;
}

template<typename T> 
void show(T e) noexcept
{
    std::cout << e << std::endl;
}

template<typename T> 
void show(const std::vector<T>& v) noexcept
{
    std::cout << "{" << concat(v) << "}" << std::endl;
}

template<typename T> 
void show(const std::vector<std::vector<T>>& vv) noexcept
{ 
    std::string s; 
    for(const auto& v : vv)
        s += " {" + concat(v) + "},\n";
    s.pop_back();
    s.pop_back();
    std::cout << "{\n" << s << "\n}" << std::endl;
}

std::string zero_fill(const int& number, const int& digits) noexcept
{
    std::string number_str = std::to_string(number);
    return std::string(std::max(0, digits-static_cast<int>(number_str.size())), '0') + number_str;
}

template<typename T>
std::string get_type(const T& t)
{
#ifdef __unix__
    int status;
    return std::string(abi::__cxa_demangle(typeid(t).name(), 0, 0, &status));
#else // Windows
    return std::string(typeid(t).name());
#endif
}

} // Utility

#endif // _UTILITY_CORE_HPP_
