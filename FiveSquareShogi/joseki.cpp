#include "joseki.h"
#include "usi.h"
#include <iostream>
#include <fstream>
#include <queue>
#include <Windows.h>
#include <climits>
#include <random>
#include "pruning.h"

HANDLE shareHandle;

Joseki::Joseki(){
	option.addOption("joseki_on", "check", "true");
	option.addOption("joseki_sokusashi_on", "check", "true");
}

void Joseki::setOption(std::vector<std::string> tokens){
	option.setOption(tokens);
	input.option.setOption(tokens);
	output.option.setOption(tokens);
	josekiDataBase.option.setOption(tokens);
	josekiByKyokumen.option.setOption(tokens);
}
void Joseki::coutOption() {
	option.coutOption();
	input.option.coutOption();
	output.option.coutOption();
	josekiDataBase.option.coutOption();
	josekiByKyokumen.option.coutOption();
}

void Joseki::init(SearchTree *tree){
	if (option.getC("joseki_on")) {
		input.josekiInput(tree);
		josekiDataBase.josekiInputFromDB(tree);
		josekiByKyokumen.init();
		sokusashi = option.getC("joseki_sokusashi_on");
	}
}

void Joseki::fin(std::vector<SearchNode*>history){
	if (option.getC("joseki_on")) {
		output.backUp(history);
		output.josekiOutput(history);
	}
}

Move Joseki::getBestMove(std::vector<SearchNode*> history){
	if (sokusashi) {
		//sokusashi = josekiByKyokumen.getBestMove(history);
	}
	else {
		return Move();
	}
}

