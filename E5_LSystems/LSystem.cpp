#include "LSystem.h"

LSystem::LSystem(string Axiom):
	m_Axiom(Axiom)
{
}

void LSystem::Run(const int count) {
	m_CurrentSystem = m_Axiom;
	for (int i = 0; i < count; i++) {
		Iterate();
	}
}

void LSystem::AddRule(char predecessor, string successor) {
	m_Rules.emplace(predecessor, successor);
}

void LSystem::Iterate() {
	string nextSystem;
	for (int i = 0; i < m_CurrentSystem.length(); i++) {
		if (m_Rules.count(m_CurrentSystem[i]) > 0) {
			//It matches a rule, so replace it
			nextSystem += m_Rules.find(m_CurrentSystem[i])->second;
		}
		else {
			//It does not match a rule, so just add it to the string
			nextSystem += m_CurrentSystem[i];
		}
	}
	m_CurrentSystem = nextSystem;	//Replace the whole string
}
