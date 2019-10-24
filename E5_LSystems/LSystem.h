#pragma once
#include <string>
#include <map>
#include "D3D.h"
using std::string;
class LSystem
{
public:
	LSystem(string Axiom);

	//Get the string that represents the current state of the L-System
	string GetCurrentSystem() { return m_CurrentSystem; }

	void AddRule(const char, const string);		//Add a rule to the system
	void Run(const int count);					//Run the system a set number of times
	void Iterate();								//Apply the rules one time

private:
	 string m_Axiom;
	 string m_CurrentSystem;
	 std::map<char, string> m_Rules;
	 
};

