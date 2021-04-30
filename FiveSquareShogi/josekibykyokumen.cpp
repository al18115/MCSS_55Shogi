#include <iostream>
#include <fstream>
#include "josekibykyokumen.h"

JosekiByKyokumen::JosekiByKyokumen(){
	option.addOption("joseki_by_kyokumen_on", "check", "true");
}

void JosekiByKyokumen::input()
{
	inputBinary();
}

void JosekiByKyokumen::output(SearchNode* root)
{
	std::vector<std::string> usitokens = {"position","startpos"};
	Kyokumen kyokumen(usitokens);
	outputRecursive(0, kyokumen, root);

	outputBinary();
	//outputText();

}

void JosekiByKyokumen::inputBinary()
{
	FILE* fp;
	fopen_s(&fp, "joseki/testkyokumen.bin", "rb");

	size_t kyokumenCount;
	fread(&kyokumenCount, sizeof(size_t), 1, fp);
	kyokumensavedata* ksd = (kyokumensavedata*)malloc(sizeof(kyokumensavedata) * kyokumenCount);
	fread(ksd, sizeof(kyokumensavedata), kyokumenCount, fp);

	fclose(fp);

	for (size_t i = 0; i < kyokumenCount; ++i) {
		kyokumenMap[kdkey(ksd[i].bammen, ksd[i].moveCount)] = kyokumendata(i, Move(ksd[i].moveU), (SearchNode::State)ksd[i].status, ksd[i].eval, ksd[i].mass);
	}

	FILE* fp2;
	fopen_s(&fp2, "joseki/testkyokumen2.bin", "wb");
	fwrite(&kyokumenCount, sizeof(size_t), 1, fp2);
	fwrite(ksd, sizeof(kyokumensavedata), kyokumenCount, fp2);
	fclose(fp2);
}

void JosekiByKyokumen::outputBinary(){
	//局面の出力
	size_t kyokumenCount = kyokumenMap.size();
	kyokumensavedata* ksd = (kyokumensavedata*)malloc(sizeof(kyokumensavedata) * kyokumenCount);
	for (auto itr = kyokumenMap.begin(); itr != kyokumenMap.end(); ++itr) {
		ksd[itr->second.kyokumenNumber] = kyokumensavedata(
			itr->first.moveCount,
			itr->first.bammen,
			itr->second.node->eval,
			itr->second.node->mass,
			itr->second.node->getState()
		);
	}
	FILE* fp;
	fopen_s(&fp, "joseki/testkyokumen.bin", "wb");
	fwrite(&kyokumenCount, sizeof(size_t), 1, fp);
	fwrite(ksd, sizeof(kyokumensavedata), kyokumenCount, fp);

	//指し手の出力
	size_t moveCount = moveMap.size();
	movesavedata* msd = (movesavedata*)malloc(sizeof(movesavedata) * moveCount);

	fclose(fp);
}

void JosekiByKyokumen::outputText(){
	std::ofstream ofs("joseki/testkyokumen.txt");
	for (auto itr = kyokumenMap.begin(); itr != kyokumenMap.end(); ++itr) {
		ofs << itr->second.kyokumenNumber << "\t";

		ofs << itr->first.moveCount << "\t";
		ofs << Kyokumen(itr->first.bammen, true).toSfen() << "\t";

		ofs << itr->second.node->eval << "\t";
		ofs << itr->second.node->mass << "\t";
		ofs << (int)itr->second.node->getState() << "\t";

		ofs << std::endl;
	}
}

size_t JosekiByKyokumen::outputRecursive(int moveCount, Kyokumen kyokumen, SearchNode* node)
{
	kdkey kk;
	kk.bammen = kyokumen.getBammen();
	kk.moveCount = moveCount;
	
	auto itr = kyokumenMap.find(kk);
	if (itr != kyokumenMap.end()) {
		if (itr->second.node->mass < node->mass) {
			itr->second.node = node;
		}
	}
	else {
		kyokumendata kd;
		kd.node = node;
		kd.kyokumenNumber = kyokumenMap.size();
		kyokumenMap[kk] = kd;
	}
	size_t kyokumenNumber = kyokumenMap[kk].kyokumenNumber;

	for (int i = 0; i < node->children.size();++i) {
		Kyokumen childKyokumen = kyokumen;
		mdkey mk;
		mk.move = node->children[i].move;
		mk.parentKyokumenNumber = kyokumenNumber;
		childKyokumen.proceed(mk.move);
		moveMap[mk] = outputRecursive(moveCount + 1, childKyokumen, &node->children[i]);
	}

	return kyokumenNumber;
}
