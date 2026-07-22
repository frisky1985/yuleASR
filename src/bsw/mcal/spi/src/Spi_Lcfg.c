/** @file Spi_Lcfg.c @brief Spi Link-Time Configuration */
#include "Spi.h"
#include "Spi_Cfg.h"
extern const Spi_ConfigType* const Spi_ConfigPtr;
extern const Spi_ConfigType Spi_Config;
const Spi_ConfigType Spi_Config = { 0U };
const Spi_ConfigType* const Spi_ConfigPtr = &Spi_Config;
