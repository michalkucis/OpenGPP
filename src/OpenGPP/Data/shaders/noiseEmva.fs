#version 130

uniform sampler2D tex;

uniform float randomSeed;

uniform float fItoP, totalQuantumEfficiently,
	readNoise, darkCurrent, inverseOfOverallSystemGain, 
    expositionTime, fDNtoI;
uniform int saturationCapacity;

//--------------------------------------------------
/*
int f2int(float x)
{
    return float(x);
}

ivec3 f2int(const in vec3 x)
{
    ivec3 v3;
    v3.x = f2int(x.x);
    v3.y = f2int(x.y);
    v3.z = f2int(x.z);
    return v3;
}

float int2f(const in int x)
{
    return float(x);
}

vec3 int2f(const in ivec3 x)
{
    vec3 v3;
    v3.x = int2f(x.x);
    v3.y = int2f(x.y);
    v3.z = int2f(x.z);
    return v3;
}

float uint2f(const in uint x)
{
    return float(x);
}

vec3 int2f(const in uvec3 x)
{
    vec3 v3;
    v3.x = uint2f(x.x);
    v3.y = uint2f(x.y);
    v3.z = uint2f(x.z);
    return v3;
}
*/
//--------------------------------------------------

float randomF (const in vec2 co)
{
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

float randomF (const in float seed)
{
    vec2 offset = gl_TexCoord[0].st;
    offset.x += seed;
    return randomF (offset);
}

vec3 random3F (const in vec3 seed)
{
    vec3 v3;
    v3.x = randomF(seed.x);
    v3.y = randomF(seed.y);
    v3.z = randomF(seed.z);
    return v3;
}

//--------------------------------------------------

#define PI 3.14159265359f

#define  A1  (-3.969683028665376e+01)
#define  A2   2.209460984245205e+02
#define  A3  (-2.759285104469687e+02)
#define  A4   1.383577518672690e+02
#define  A5  (-3.066479806614716e+01)
#define  A6   2.506628277459239e+00

#define  B1  (-5.447609879822406e+01)
#define  B2   1.615858368580409e+02
#define  B3  (-1.556989798598866e+02)
#define  B4   6.680131188771972e+01
#define  B5  (-1.328068155288572e+01)

#define  C1  (-7.784894002430293e-03)
#define  C2  (-3.223964580411365e-01)
#define  C3  (-2.400758277161838e+00)
#define  C4  (-2.549732539343734e+00)
#define  C5   4.374664141464968e+00
#define  C6   2.938163982698783e+00

#define  D1   7.784695709041462e-03
#define  D2   3.224671290700398e-01
#define  D3   2.445134137142996e+00
#define  D4   3.754408661907416e+00

#define P_LOW   0.02425
#define P_HIGH  0.97575

float inverseERF(float p)
{
    float x;
    if ((0.0 < p) && (p < P_LOW))
    {
        float q = sqrt(-2.0*log(p));
        x = (((((C1*q+C2)*q+C3)*q+C4)*q+C5)*q+C6) / ((((D1*q+D2)*q+D3)*q+D4)*q+1.0);
    }
    else{
        if ((P_LOW <= p) && (p <= P_HIGH)){
           float q = p - 0.5;
           float r = q*q;
           x = (((((A1*r+A2)*r+A3)*r+A4)*r+A5)*r+A6)*q /(((((B1*r+B2)*r+B3)*r+B4)*r+B5)*r+1.0);
        }
        else{
            if ((P_HIGH < p)&&(p < 1.0)){
                float q = sqrt(-2.0*log(1.0-p));
                x = -(((((C1*q+C2)*q+C3)*q+C4)*q+C5)*q+C6) / ((((D1*q+D2)*q+D3)*q+D4)*q+1.0);
            }
        }
    }
    return x;
}

float normalQuantile(float p, float mi, float o)
{
    return mi + sqrt(o)*sqrt(2.0)*inverseERF(2.0*p-1.0);
}

float binomialQuantile(int k, float n, float p)
{
    return normalQuantile(float(k), n*p, sqrt(n*p*(1.0-p)));
}

float photonsToElectrons(float photons, float randomSeed)
{
    float f = randomF(randomSeed);
    float electrons = binomialQuantile(
        int(totalQuantumEfficiently*photons),
        photons, 
        f);
    return electrons;
}

//--------------------------------------------------

const int g_cycles = 100000;

/*
float giveMeNoise (const in float value, float randomSeed)
{
    float photons = value * fItoP;
    float electrons = photonsToElectrons(photons, randomSeed);
    return electrons / fItoP;
}
*/

uint rng_state;
uint rand_lcg()
{
    // LCG values from Numerical Recipes
    rng_state = 1664525u * rng_state + 1013904223u;
    return rng_state;
}
 
uint rand_xorshift()
{
    // Xorshift algorithm from George Marsaglia's paper
    rng_state ^= (rng_state << 13);
    rng_state ^= (rng_state >> 17);
    rng_state ^= (rng_state << 5);
    return rng_state;
}

float rand()
{
    uint v = rand_lcg();
    return float(rand_xorshift())/float(uint(-1));
}

void initRand()
{
/*    
    float fnorm = fract(gl_TexCoord[0].s * (1 + gl_TexCoord[0].t) + randomSeed);

    uint seed = uint(fnorm * (1 << 31));
    seed = (seed ^ 61u) ^ (seed >> 16u);
    seed *= 9u;
    seed = seed ^ (seed >> 4);
    seed *= 0x27d4eb2du;
    seed = seed ^ (seed >> 15);
    rng_state = seed;
 */

    float fnorm = fract(fract(sin(dot(gl_TexCoord[0].st, vec2(12.9898, 78.233))) * 43758.5453) + randomSeed);
    rng_state = uint(fnorm * float(uint(-1)));
}

int getPoissonRandomValue(float mean)
{
    float R;
    float sum = 0;
    int i=-1;
    float z;
 
    while(sum <= mean)
    {
        R = rand();
        z = -log(R);
        sum += z;
        i++;
    }
    return i;
}

int getDarkAndReadNoise()
{
    float darkNoise = darkCurrent * expositionTime;
    float mean = sqrt(readNoise*readNoise + darkNoise*darkNoise);
    return getPoissonRandomValue(mean);
}

int bruteForcePhotonsToElectrons(int photons)
{
    float seed = fract(gl_TexCoord[0].s * (1+gl_TexCoord[0].t) + randomSeed);
    int electrons = 0;
    for(int i = 0; i < g_cycles; i++)
        if (i < photons)
        {
            bool effective = rand() <= totalQuantumEfficiently;
            electrons += effective ? 1 : 0;
        }
    return electrons;
}

float giveMeNoise (const in float value)
{
    float photons = value * fItoP;
    int electrons = bruteForcePhotonsToElectrons(int(photons+0.5));
    electrons += getDarkAndReadNoise();
    electrons = electrons > saturationCapacity ? saturationCapacity : electrons;
    return electrons / fItoP / totalQuantumEfficiently;
}

void main ()  
{
    initRand();

	vec4 color = texture2D(tex, gl_TexCoord[0].st);
    color.r = giveMeNoise(color.r);
    color.g = giveMeNoise(color.g);
    color.b = giveMeNoise(color.b);
//    color.r = rng_state;//rand();
//    color.g = color.r;
//    color.b = color.r;
	gl_FragColor = color;
}