#ifndef MATH_H
#define MATH_H
#include "stdint.h"

#define E 2.71828
#define PI 3.14159265358979323846264338327950

uint32_t rand();
double fmod(double x, double m);
double fabs(double x);
double sin(double x);
double cos(double x);
double pow(double x, double y);

#endif