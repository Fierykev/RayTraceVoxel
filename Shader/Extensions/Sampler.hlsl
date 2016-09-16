// store the two sample states here

SamplerState linearsampler : register(s0);

SamplerComparisonState comparesamples// : register(s1);
{
	// sampler state
	Filter = COMPARISON_MIN_MAG_MIP_LINEAR;
	AddressU = MIRROR;
	AddressV = MIRROR;

	// sampler comparison state
	ComparisonFunc = LESS_EQUAL;
};


SamplerState pointsampler : register(s2);