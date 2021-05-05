#include <iostream>
#include <fstream>
#include "josekibykyokumen.h"
#include "usi.h"

static class StopWatch {
public:
	void start() {
		if (isStop) {
			isStop = false;
			starttime = std::chrono::system_clock::now();
		}
	}
	void stop() {
		if (!isStop) {
			elapsed += std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - starttime).count();
			isStop = true;
		}
	}
	void print(std::string before, std::string after) {
		double e = elapsed;
		if (!isStop) {
			e += std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - starttime).count();
		}
		std::cout << before << e << after << std::endl;
	}
	void print(std::string before) {
		print(before, "");
	}
	void print() {
		print("");
	}
private:
	bool isStop = true;
	double elapsed = 0;
	std::chrono::system_clock::time_point starttime;
};

JosekiByKyokumen::JosekiByKyokumen(){
	option.addOption("joseki_by_kyokumen_on", "check", "true");
	option.addOption("joseki_by_kyokumen_multithread", "check", "true");
}

void JosekiByKyokumen::input(SearchTree *tree)
{
	StopWatch sw;
	sw.start();
	inputBinary();
	sw.print("inputBinaly:");
	SearchNode* root = new SearchNode;

	buildTree(root, 0, Move().binary(), option.getC("joseki_by_kyokumen_multithread"));

	sw.print("buildTree:");
	tree->setRoot(root);
}

void JosekiByKyokumen::inputBinary()
{
	FILE* fp;
	if (fopen_s(&fp, "joseki/testKyokumen.bin", "rb") != 0) {
		std::cout << "file open error." << std::endl;
		return;
	}

	//局面情報の読み込み
	size_t kyokumenCount;
	fread(&kyokumenCount, sizeof(size_t), 1, fp);
	std::vector<kyokumensavedata>ksd;
	ksd.resize(kyokumenCount);
	fread(&ksd[0], sizeof(kyokumensavedata), kyokumenCount, fp);

	//指し手情報の読み込み
	size_t childrenCount;
	fread(&childrenCount, sizeof(size_t), 1, fp);
	std::vector<movesavedata> msd;
	msd.resize(childrenCount);
	fread(&msd[0], sizeof(movesavedata), childrenCount, fp);

	fclose(fp);

	std::vector<std::vector<kyokumendata::child>> kyokumenChildren;
	kyokumenChildren.resize(kyokumenCount);
	for (size_t i = 0; i < childrenCount; ++i) {
		kyokumenChildren[msd[i].parentKyokumenNumber].push_back(kyokumendata::child(msd[i].moveU,msd[i].childKyokmenNumber));
	}

	for (size_t i = 0; i < kyokumenCount; ++i) {
		const auto& ksdi = ksd[i];
		auto key = kdkey(ksdi.bammen, ksdi.moveCount);
		kyokumenMap[key] = kyokumendata(ksdi.ID, (SearchNode::State)ksdi.status, ksdi.eval, ksdi.mass);
		kyokumenMap[key].children = kyokumenChildren[ksdi.ID];
		keyList[ksdi.ID] = key;
	}

}

void JosekiByKyokumen::buildTree(SearchNode* node, size_t ID,uint16_t moveU,bool multiThread)
{
	const auto& km = kyokumenMap[keyList[ID]];
	node->restoreNode(Move(moveU), km.state, km.eval, km.mass);
	size_t childCount = km.children.size();
	SearchNode* list = new SearchNode[childCount];
	if (multiThread) {
		std::vector<std::thread>th;
		for (size_t i = 0; i < childCount; ++i) {
			th.push_back(std::thread(&JosekiByKyokumen::buildTree, this, &(list[i]), km.children[i].ID, km.children[i].moveU,false));
		}
		for (size_t i = 0; i < th.size(); ++i) {
			th[i].join();
		}
	}
	else {
		for (size_t i = 0; i < childCount; ++i) {
			buildTree(&(list[i]), km.children[i].ID, km.children[i].moveU,false);
		}
	}
	node->children.setChildren(list,childCount);
}

void JosekiByKyokumen::output(SearchNode* root)
{
	StopWatch sw;
	sw.start();

	std::vector<std::string> usitokens = { "position","startpos" };
	Kyokumen kyokumen(usitokens);
	size_t rootID = 0;
	outputRecursive(0, kyokumen, root,&rootID, option.getC("joseki_by_kyokumen_multithread"));

	sw.print("outputRecurcive:");

	outputBinary();

	sw.print("outputBinary:");
}


void JosekiByKyokumen::outputBinary() {
	//局面の出力
	std::vector<kyokumensavedata> ksd;
	ksd.reserve(kyokumenMap.size());
	std::vector<movesavedata>msd;

	for (auto itr = kyokumenMap.begin(); itr != kyokumenMap.end(); ++itr) {
		ksd.push_back(
			kyokumensavedata(
				itr->second.ID,
				itr->first.hisCount,
				itr->first.bammen,
				itr->second.eval,
				itr->second.mass,
				itr->second.state
			)
		);
		for (int i = 0; i < itr->second.children.size();++i) {
			msd.push_back(movesavedata(itr->second.ID,itr->second.children[i].moveU, itr->second.children[i].ID));
		}
		if (itr->second.ID == 0) {
			std::cout << std::endl;
		}
	}
	
	FILE* fp;
	if (fopen_s(&fp, "joseki/testkyokumen.bin", "wb") != 0) {
		std::cout << "file open error." << std::endl;
		return;
	}

	size_t kyokumenCount = ksd.size();
	fwrite(&kyokumenCount, sizeof(size_t), 1, fp);
	fwrite(&ksd[0], sizeof(kyokumensavedata), kyokumenCount, fp);

	size_t childrenCount = msd.size();
	fwrite(&childrenCount, sizeof(size_t), 1, fp);
	fwrite(&msd[0], sizeof(movesavedata), childrenCount, fp);

	fclose(fp);

	bool outputtexton = true;
	if (outputtexton) {
		std::vector<std::string>outputs;
		outputs.resize(kyokumenCount);
		for (size_t i = 0; i < kyokumenCount; ++i) {
			outputs[ksd[i].ID] = "";
			outputs[ksd[i].ID] += std::to_string(ksd[i].ID) + "\t";
			outputs[ksd[i].ID] += Kyokumen(ksd[i].bammen, (ksd[i].moveCount + 1) % 2).toSfen() + "\t";
			outputs[ksd[i].ID] += std::to_string(ksd[i].moveCount) + "\t";
			outputs[ksd[i].ID] += std::to_string(ksd[i].status) + "\t";
			outputs[ksd[i].ID] += std::to_string(ksd[i].eval) + "\t";
			outputs[ksd[i].ID] += std::to_string(ksd[i].mass) + "\n";
		}

		for (size_t i = 0; i < childrenCount; ++i) {
			size_t ID = msd[i].parentKyokumenNumber;
			size_t childID = msd[i].childKyokmenNumber;
			Move m = Move(msd[i].moveU);
			outputs[ID] += std::to_string(ID) + "\t"+ std::to_string(m.binary()) + "(" + m.toUSI() + ")" + "\t"+std::to_string(childID)  + "\n";
		}
		std::ofstream ofs("joseki/testkyokumen.txt");
		ofs << "kyokumenCount:" << kyokumenCount << std::endl;
		ofs << "childrenCount:" << childrenCount << std::endl;
		for (size_t i = 0; i < kyokumenCount; ++i) {
			ofs << outputs[i];
		}
		fclose(fp);
	}
}

void JosekiByKyokumen::outputRecursive(int hisCount, Kyokumen kyokumen, SearchNode* node,size_t *ID,bool multiThread)
{
	kdkey key(kyokumen.getBammen(), hisCount);
	
	bool duplicate,checkChildren = true;

	auto itr = kyokumenMap.find(key);
	duplicate = (itr != kyokumenMap.end());

	kyokumendata* kd;
	if (duplicate) {
		//既に局面が保存されている
		kd = &kyokumenMap[key];
		*ID = kd->ID;
		if (kd->mass < node->mass) {
			kd->eval = node->eval;
			kd->mass = node->mass;
		}
		else {
			checkChildren = false;
		}
	}
	else {
		//新しく局面を保存する
		*ID = kyokumenMap.size();
		kyokumendata newkd(*ID, node->getState(), node->eval, node->mass);
		kyokumenMap[key] = newkd;
		kd = &kyokumenMap[key];
		keyList[*ID] = key;
	}

	if (checkChildren = false) {
		if(!duplicate) kd->children.resize(node->children.size());
		if (multiThread) {
			std::vector<std::thread>th;
			for (int i = 0; i < node->children.size(); ++i) {
				SearchNode* child = &(node->children[i]);
				Kyokumen childKyokumen = Kyokumen(kyokumen);
				childKyokumen.proceed(child->move);
				th.push_back(std::thread(&JosekiByKyokumen::outputRecursive, this, hisCount + 1, childKyokumen, child, &(kd->children[i].ID), false));
				kd->children[i].moveU = child->move.binary();
			}
			for (int i = 0; i < th.size(); ++i) {
				th[i].join();
			}
		}
		else {
			for (int i = 0; i < node->children.size(); ++i) {
				SearchNode* child = &(node->children[i]);
				Kyokumen childKyokumen = Kyokumen(kyokumen);
				childKyokumen.proceed(child->move);
				outputRecursive(hisCount + 1, childKyokumen, child,&(kd->children[i].ID),false);
				kd->children[i].moveU = child->move.binary();
			}
		}
	}
}
