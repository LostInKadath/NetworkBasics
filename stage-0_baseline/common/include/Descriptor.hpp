#pragma once
#include <set>

namespace NetworkBasics::common
{
    struct Descriptor
    {
        static const int InvalidValue;

        Descriptor(int value);
        
        Descriptor(Descriptor&&);
        Descriptor& operator=(Descriptor&&);

        Descriptor(const Descriptor&) = delete;
        Descriptor& operator=(const Descriptor&) = delete;

        ~Descriptor();

        int operator*() const;

    private:
        int m_value{ InvalidValue };
    };

    struct DescriptorComparator
    {
        using is_transparent = void;

        bool operator()(const Descriptor& lhs, const Descriptor& rhs) const;
        bool operator()(const Descriptor& lhs, int rhs) const;
        bool operator()(int lhs, const Descriptor& rhs) const;
    };

    using DescriptorSet = std::set<Descriptor, DescriptorComparator>;
}