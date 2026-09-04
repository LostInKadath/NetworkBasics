#pragma once

namespace NetworkBasics::common
{
    struct Descriptor
    {
        static const int InvalidValue;

        Descriptor(int value);
        
        Descriptor(Descriptor&&);
        Descriptor& operator=(Descriptor&& other);

        Descriptor(const Descriptor&) = delete;
        Descriptor& operator=(const Descriptor&) = delete;

        ~Descriptor();

        int operator*() const;

        bool operator<(const Descriptor& other) const;

    private:
        int m_value{ InvalidValue };
    };
}