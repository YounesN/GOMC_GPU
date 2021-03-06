
#include "OutConst.h" //For namespace spec;

namespace out
{
   const std::string ENERGY_TOTAL = "EnergyTotal";
#ifdef EN_SUBCAT_OUT
   const std::string ENERGY_INTER = "EnergyInter";
   const std::string ENERGY_TC = "EnergyTC";
   const std::string ENERGY_INTRA_B  = "EnergyIntraBond";
   const std::string ENERGY_INTRA_NB = "EnergyIntraNonbond";
#endif
   const std::string VIRIAL_TOTAL = "VirialTotal";
#ifdef VIR_SUBCAT_OUT
   const std::string VIRIAL_INTER = "VirialInter";
   const std::string VIRIAL_TC = "VirialTC";
#endif
   const std::string PRESSURE = "Pressure";
#ifdef VARIABLE_VOLUME
   const std::string VOLUME = "Volume";
#endif
#if ENSEMBLE == GEMC
   const std::string HEAT_OF_VAP = "HeatOfVaporization";
#endif
#ifdef VARIABLE_DENSITY
   const std::string DENSITY = "Density";
#endif
#ifdef VARIABLE_PARTICLE_NUMBER
   const std::string MOL_NUM = "MolNum";
   const std::string MOL_FRACTION = "MolFraction";
#endif
}

