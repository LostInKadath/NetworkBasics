#include "Descriptor.hpp"

#include <cstring>  
#include <iostream>
#include <utility>

#include <unistd.h>

using namespace NetworkBasics::common;

const int Descriptor::InvalidValue = -1;

Descriptor::Descriptor(int value)
    : m_value(value)
{}

Descriptor::Descriptor(Descriptor&& other)
    : m_value{ std::exchange(other.m_value, Descriptor::InvalidValue) }
{}

Descriptor& Descriptor::operator=(Descriptor&& other)
{
    m_value = std::exchange(other.m_value, Descriptor::InvalidValue);
    return *this;
}

Descriptor::~Descriptor()
{
    if (m_value < 0)
        return;
    if (const auto res = close(m_value); res != 0)
        std::cerr << "Error while closing descriptor " << m_value << ": " << std::strerror(errno) << '\n';
    else
        std::cout << "Descriptor " << m_value << " closed.\n";
}

int Descriptor::operator*() const
{
    return m_value;
}

bool DescriptorComparator::operator()(const Descriptor& lhs, const Descriptor& rhs) const
{
    return *lhs < *rhs;
}

bool DescriptorComparator::operator()(const Descriptor& lhs, int rhs) const
{
    return *lhs < rhs;
}

bool DescriptorComparator::operator()(int lhs, const Descriptor& rhs) const
{
    return lhs < *rhs;
}
