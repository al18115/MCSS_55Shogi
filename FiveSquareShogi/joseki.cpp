#include "joseki.h"
#include "usi.h"
#include <iostream>
#include <fstream>
#include <queue>
#include <Windows.h>
#include <climits>
#include <random>

HANDLE shareHandle;

Joseki::Joseki(){
	option.addOption("joseki_on", "check", "false");
}

void Joseki::setOption(std::vector<std::string> tokens){
	option.setOption(tokens);
	input.option.setOption(tokens);
	output.option.setOption(tokens);
	josekiDataBase.option.setOption(tokens);
}
void Joseki::coutOption() {
	option.coutOption();
	input.option.coutOption();
	output.option.coutOption();
	josekiDataBase.option.coutOption();
}

void Joseki::init(SearchTree *tree){
	if (option.getC("joseki_on")) {
		input.josekiInput(tree);
		josekiDataBase.josekiInputFromDB(tree);
	}
}

void Joseki::fin(std::vector<SearchNode*>history){
	if (option.getC("joseki_on")) {
		output.backUp(history);
		output.josekiOutput(history);
		josekiDataBase.josekiOutput(history.front());
	}
}
