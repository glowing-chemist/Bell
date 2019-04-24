#ifndef PASSTYPES_HPP
#define PASSTYPES_HPP

#include <cstdint>

// An enum to keep track of which 
enum class PassType : uint64_t
{
	DepthPre = 0,
	Albedo = 1 << 1,
	Normal = 1 << 2,
	Position = 1 << 3,
	Animation = 1 << 4,
	DeferredLighting = 1 << 5,
	Shadow = 1 << 6,
	CascadingShadow = 1 << 7

	// Add more asw and when implemented.
};

#endif

