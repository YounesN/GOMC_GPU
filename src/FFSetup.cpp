#include "FFSetup.h"
#include <algorithm>
#include <iostream>

#ifndef NDEBUG
#include <numeric>
#endif

#include "../lib/GeomLib.h"


const uint FFSetup::CHARMM_ALIAS_IDX = 0;
const uint FFSetup::EXOTIC_ALIAS_IDX = 1;
const std::string FFSetup::paramFileAlias[] = 
{"CHARMM-Style Parameter File", "Exotic Parameter File"}; 
const double ff_setup::KCAL_PER_MOL_TO_K = 503.21959899;
const double ff_setup::RIJ_OVER_2_TO_SIG = 1.7817974362807;
const double ff_setup::RIJ_TO_SIG = 0.890898718;

const double ff_setup::Bond::FIXED = 99999999;

//Map variable names to functions
   std::map<std::string, ReadableBaseWithFirst *>  
FFSetup::SetReadFunctions(const bool isCHARMM)
{
   std::map<std::string, ReadableBaseWithFirst *> funct;
   //From CHARMM style file.
   funct["BONDS"] = &bond;
   funct["ANGLES"] = &angle;
   funct["DIHEDRALS"] = &dih;
   funct["IMPROPERS"] = &imp;
   if (isCHARMM)
   {
      funct["NONBONDED"] = &mie;
      funct["NBFIX"] = &nbfix;
   }
   else
   {
      //Unique to exotic file
      funct["NONBONDED"] = &mie;
      funct["NONBONDED_MIE"] = &mie;
      funct["NBFIX"] = &nbfix; 
      funct["NBFIX_MIE"] = &nbfix;
   }
   for (sect_it it = funct.begin(); it != funct.end(); ++it)
   {
      (dynamic_cast<ff_setup::FFBase *>(it->second))->IsCHARMM(isCHARMM);
   }
   return funct;
}

void FFSetup::Init(std::string const& name, const bool isCHARMM)
{
   using namespace std;
   sectKind = SetReadFunctions(isCHARMM);
   string currSectName="", varName="";
   string commentChar = "*!";
   string commentStr = "REMARK set AEXP REXP HAEX AAEX NBOND "
      "CUTNB END CTONN EPS VSWI NBXM INHI";
   map<string, ReadableBaseWithFirst *>::const_iterator sect, currSect;

   Reader param(name, 
		paramFileAlias[isCHARMM?CHARMM_ALIAS_IDX:EXOTIC_ALIAS_IDX], 
		true, &commentStr, true, &commentChar);
   param.open();
   while (param.Read(varName))
   {
      sect = sectKind.find(varName);
      if ( sect != sectKind.end() )
      {
	 param.SkipLine(); //Skip rest of line for sect. heading
	 currSectName = varName;
	 currSect = sect; //Save for later calls.
      }
      else
	 currSect->second->Read(param, varName);
   }
   param.close();
   //Adjust dih names so lookup finds kind indices rather than term counts
   std::vector<std::string>::iterator newEnd;
   newEnd = std::unique(dih.name.begin(), dih.name.end());
   dih.name.erase(newEnd, dih.name.end());

#ifndef NDEBUG
   if (isCHARMM)
   {
      std::cout << "Lennard-Jones Particles:\n";
   }
   else
   {
      std::cout << "Mie Particles:\n";
   }
   mie.PrintBrief();
   std::cout << "Bonds:\n";
   bond.PrintBrief();
   std::cout << "Angles:\n";
   angle.PrintBrief();
   std::cout << "Dihedrals:\n";
   dih.PrintBrief();
#endif
}

namespace ff_setup
{

   std::string FFBase::ReadKind(Reader & param, 
				std::string const& firstKindName) 
   { 
      std::string merged=firstKindName, tmp;
      for (uint k = 1; k < numTerms; k++)
      {
	 param.file >> tmp;
	 merged += tmp;
      }
      if (!multi ||
	  std::find(name.begin(), name.end(), merged) == name.end())
	 name.push_back(merged);
      return merged;
   }

   std::string FFBase::LoadLine(Reader & param, std::string const& firstVar)
   {
      std::string line;
      ReadKind(param, firstVar);
      std::getline(param.file, line);
      return line;
   }

   void Particle::Read(Reader & param, std::string const& firstVar)
   {
      double e, s, e_1_4, s_1_4, dummy1, dummy2;
      uint expN, expN_1_4;
      std::stringstream values(LoadLine(param, firstVar));      
      if (isCHARMM) //if lj
      {
	 values >> dummy1;
      } 
      values >> e >> s;
      if (isCHARMM)
      {
	 expN = ff::part::lj_n;

      }
      else
      {
	 values >> expN;
      }
      //If undefined in CHARMM, assign 1-4 to full value.
      values >> dummy2 >> e_1_4 >> s_1_4;
      if (values.fail())
      {
	 e_1_4 = e;
	 s_1_4 = s; 
      }
      values >> expN_1_4; 
      if (isCHARMM || values.fail())
      {
	 expN_1_4 = expN;
      }
      Add(e, s, expN, e_1_4, s_1_4, expN_1_4);
   }

   void Particle::Add(double e, double s, const uint expN,
		      double e_1_4, double s_1_4, const uint expN_1_4)
   {
      if (isCHARMM)
      {
	 e *= -1.0;
	 s *= RIJ_OVER_2_TO_SIG;
	 e_1_4 *= -1.0;
	 s_1_4 *= RIJ_OVER_2_TO_SIG;
      }
      epsilon.push_back(EnConvIfCHARMM(e));
      sigma.push_back(s);
      n.push_back(expN);
      epsilon_1_4.push_back(EnConvIfCHARMM(e_1_4));
      sigma_1_4.push_back(s_1_4);
      n_1_4.push_back(expN_1_4);
   }

   void NBfix::Read(Reader & param, std::string const& firstVar)
   {
      double e, s, e_1_4, s_1_4;

#ifdef MIE_INT_ONLY
      uint expN, expN_1_4;
#else
      double expN, expN_1_4;
#endif

   
      std::stringstream values(LoadLine(param, firstVar));

      values >> e >> s;
      if (isCHARMM)
      {
	 expN = ff::part::lj_n;
      }
      else
      {

	 values >> expN;
      }

      
       values >> e_1_4 >> s_1_4;
      if (values.fail())
      {

	 e_1_4 = e;
	 s_1_4 = s; 
      }

      values >> expN_1_4; 
      if (isCHARMM || values.fail())
      {

	 expN_1_4 = expN;
      }

      Add(e, s, expN, e_1_4, s_1_4, expN_1_4);
   }

   void NBfix::Add(double e, double s,
#ifdef MIE_INT_ONLY
const uint expN,
#else
const double expN,
#endif
	       double e_1_4, double s_1_4,
#ifdef MIE_INT_ONLY
const uint expN_1_4
#else
const double expN_1_4
#endif
	       )
   {
      if (isCHARMM)
      {
	 e *= -1.0;

	 s *= RIJ_TO_SIG;
	 e_1_4 *= -1.0;
	 s_1_4 *= RIJ_TO_SIG;
      }
      epsilon.push_back(EnConvIfCHARMM(e));
      sigma.push_back(s);
      n.push_back(expN);
      epsilon_1_4.push_back(EnConvIfCHARMM(e_1_4));
      sigma_1_4.push_back(s_1_4);
      n_1_4.push_back(expN_1_4);

   }

   void Bond::Read(Reader & param, std::string const& firstVar)
   {
      double coeff, def;
      ReadKind(param, firstVar);
      param.file >> coeff >> def;
      Add(coeff, def);
   }
   void Bond::Add(const double coeff, const double def)
   {
      fixed.push_back(coeff>FIXED);
      Kb.push_back(EnConvIfCHARMM(coeff));
      b0.push_back(def);
   }

   void Angle::Read(Reader & param, std::string const& firstVar)
   {
      double coeff, def, coeffUB, defUB;
      bool hsUB;
      std::stringstream values(LoadLine(param, firstVar));
      values >> coeff >> def;
      values >> coeffUB >> defUB; 

      hsUB = !values.fail();
      Add(coeff, def, hsUB, coeffUB, defUB);
   }
   void Angle::Add(const double coeff, const double def, const bool hsUB,
		   const double coeffUB, const double defUB)
   {
      Ktheta.push_back(EnConvIfCHARMM(coeff));
      theta0.push_back(geom::DegToRad(def));
      hasUB.push_back(hsUB);
      if (hsUB)
      {
	 Kub.push_back(EnConvIfCHARMM(coeffUB));
	 bUB0.push_back(defUB);
      }
      else
      {
	 Kub.push_back(0.0);
	 bUB0.push_back(0.0);
      }
   }

   void Dihedral::Read(Reader & param, std::string const& firstVar)
   {
      double coeff, index, def;
      std::string merged = ReadKind(param, firstVar);
      param.file >> coeff >> index >> def;
      Add(merged, coeff, index, def);
      last = merged;
   }
   void Dihedral::Add(std::string const& merged,
		      const double coeff, const uint index, const double def)
   {
      ++countTerms;
      Kchi[merged].push_back(EnConvIfCHARMM(coeff));
      n[merged].push_back(index);
      delta[merged].push_back(geom::DegToRad(def));
   }

   void Improper::Read(Reader & param, std::string const& firstVar)
   {
      double coeff, def;
      std::string merged = ReadKind(param, firstVar);
      //If new value
      if (std::find(name.begin(), name.end(), merged) == name.end())
      {
	 param.file >> coeff >> def;
	 Add(coeff, def);
      }
   }
   void Improper::Add(const double coeff, const double def)
   {
      Komega.push_back(EnConvIfCHARMM(coeff));
      omega0.push_back(def);
   }

#ifndef NDEBUG
   void Particle::PrintBrief()
   {
      std::cout << "\tSigma\t\tEpsilon\t\tN\n";
      std::cout << "#Read\t" << sigma.size() << '\t' << '\t' << epsilon.size() 
		<< "\t\t" << n.size() << '\n';
      std::cout << "Avg.\t" <<
	 std::accumulate(sigma.begin(), sigma.end(), 0.0) / sigma.size() 
		<< "\t\t" <<
	 std::accumulate(epsilon.begin(), epsilon.end(), 0.0) / epsilon.size() 
		<< "\t\t" <<
	 std::accumulate(n.begin(), n.end(), 0.0) / n.size() << "\n\n";
   }

   void Bond::PrintBrief()
   {
      std::cout << "\tKb\t\tb0\n";
      std::cout << "#Read\t" << Kb.size() << '\t' << '\t' << b0.size() << '\n';
      std::cout << "Avg.\t" <<
	 std::accumulate(Kb.begin(), Kb.end(), 0.0) / Kb.size() << "\t" <<
	 std::accumulate(b0.begin(), b0.end(), 0.0) / b0.size() << "\n\n";
   }

   void Angle::PrintBrief()
   {
      std::cout << "\tKtheta\t\ttheta0\n";
      std::cout << "#Read\t" << Ktheta.size() << '\t' << '\t' 
		<< theta0.size() << '\n';
      std::cout << "Avg.\t" <<
	 std::accumulate(Ktheta.begin(), Ktheta.end(), 0.0) / Ktheta.size() 
		<< "\t\t" <<
	 std::accumulate(theta0.begin(), theta0.end(), 0.0) / theta0.size() 
		<< "\n\n";
   }

   void Dihedral::PrintBrief()
   {
      std::cout << name.size() << " dihedrals.\n";
   }

   void Improper::PrintBrief()
   {
      std::cout << "\tKomega\t\tomega0\n";
      std::cout << "#Read\t" << Komega.size() << '\t' << '\t' 
		<< omega0.size() << '\n';
      std::cout << "Avg.\t" <<
	 std::accumulate(Komega.begin(), 
			 Komega.end(), 0.0) / Komega.size() << "\t" <<
	 std::accumulate(omega0.begin(),
			 omega0.end(), 0.0) / omega0.size() << "\n\n";
   }
#endif

} //end namespace ff_setup
