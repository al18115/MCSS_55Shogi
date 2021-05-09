#include <iostream>
#include <fstream>
#include "josekibykyokumen.h"
#include "usi.h"
#include "pruning.h"

#define FILENAME (option.getS("joseki_by_kyokumen_folder") + "/" + option.getS("joseki_by_kyokumen_filename"))
#define FILENAMETEXT (option.getS("joseki_by_kyokumen_folder") + "/" + option.getS("joseki_by_kyokumen_filenametext"))
#define FILERENAME (option.getS("joseki_by_kyokumen_folder") + "/" + JosekiOption::getYMHM() + option.getS("joseki_by_kyokumen_filename"))
#define FILERENAMETEXT (option.getS("joseki_by_kyokumen_folder") + "/" + JosekiOption::getYMHM() + option.getS("joseki_by_kyokumen_filenametext"))


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
	option.addOption("joseki_by_kyokumen_on", "check", "false");
	option.addOption("joseki_by_kyokumen_folder", "string", "joseki");
	option.addOption("joseki_by_kyokumen_filename", "string", "jdk.bin");
	option.addOption("joseki_by_kyokumen_filenametext", "string", "jdk.txt");
	option.addOption("joseki_by_kyokumen_text_output_on", "check", "false");
	option.addOption("joseki_by_kyokumen_multithread", "check", "false");
	option.addOption("joseki_by_kyokumen_mass_rate", "string", "0.5");
	option.addOption("joseki_by_kyokumen_rename", "check", "false");
}

std::atomic<size_t> nodeCounter = 0;
void JosekiByKyokumen::input(SearchTree *tree)
{
	bool inputOK = false;
	if (!option.getC("joseki_by_kyokumen_on")) {
		return;
	}

	StopWatch sw;
	sw.start();
	inputOK = inputBinary();
	sw.print("inputBinaly:");
	SearchNode* root = new SearchNode;

	if (inputOK) {
		buildTree(root, 0, Move().binary(), option.getC("joseki_by_kyokumen_multithread"));

		sw.print("buildTree:");
		tree->setRoot(root);

		if (option.getC("joseki_by_kyokumen_rename")) {
			auto r = std::rename(FILENAME.c_str(), FILERENAME.c_str());
			r = std::rename(FILENAMETEXT.c_str(), FILERENAMETEXT.c_str());
		}
	}

	std::cout << "node count : " << nodeCounter << std::endl;
}

bool JosekiByKyokumen::inputBinary()
{
	FILE* fp;
	if (fopen_s(&fp, FILENAME.c_str(), "rb") != 0) {
		std::cout << "file open error." << std::endl;
		return false;
	}

	//局面情報の読み込み
	size_t kyokumenCount;
	fread_s(&kyokumenCount, sizeof(size_t),sizeof(size_t), 1, fp);
	std::vector<kyokumensavedata>ksd;
	ksd.resize(kyokumenCount);
	fread_s(&ksd[0], sizeof(kyokumensavedata) * kyokumenCount, sizeof(kyokumensavedata), kyokumenCount, fp);

	//指し手情報の読み込み
	size_t childrenCount;
	fread_s(&childrenCount,sizeof(size_t), sizeof(size_t), 1, fp);
	std::vector<movesavedata> msd;
	msd.resize(childrenCount);
	fread_s(&msd[0], sizeof(movesavedata) * childrenCount, sizeof(movesavedata), childrenCount, fp);

	fclose(fp);

	std::vector<std::vector<kyokumendata::child>> kyokumenChildren;
	kyokumenChildren.resize(kyokumenCount);
	for (size_t i = 0; i < childrenCount; ++i) {
		kyokumenChildren[msd[i].parentKyokumenNumber].push_back(kyokumendata::child(msd[i].moveU,msd[i].childKyokmenNumber));
	}

	kyokumenDataArray = (kyokumendata*)malloc(sizeof(kyokumendata) * kyokumenCount);
	for (size_t i = 0; i < kyokumenCount; ++i) {
		const auto& ksdi = ksd[i];
		kdkey key = kdkey(ksdi.bammen, ksdi.moveCount);
		kyokumenMap[key] = kyokumendata(ksdi.ID, (SearchNode::State)ksdi.status, ksdi.eval, ksdi.mass);
		if (ksdi.status == (int)SearchNode::State::E) {
			kyokumenMap[key].children = kyokumenChildren[ksdi.ID];
		}
		keyList[ksdi.ID] = key;
		if (kyokumenDataArray != NULL) {
			kyokumenDataArray[ksdi.ID] = kyokumenMap[key];
		}
		else {
			std::cout << "メモリの確保に失敗しました" << std::endl;
			exit(1);
		}
	}

	return true;
}

std::atomic<int> threadCounter = 0;
void JosekiByKyokumen::buildTree(SearchNode* node, size_t ID, uint16_t moveU, bool multiThread)
{
	++nodeCounter;

	const auto& km = kyokumenDataArray[ID];
	//const auto& km = kyokumenMap[keyList[ID]];
	auto state = km.state;
	size_t childCount = km.children.size();
	SearchNode* list = new SearchNode[childCount];
	if (multiThread) {
		std::vector<std::thread>th;
		threadCounter.store(threadCounter + childCount);
		for (size_t i = 0; i < childCount; ++i) {
			th.push_back(std::thread(&JosekiByKyokumen::buildTree, this, &(list[i]), km.children[i].ID, km.children[i].moveU, false));
		}
		for (size_t i = 0; i < th.size(); ++i) {
			th[i].join();
		}
	}
	else {
		for (size_t i = 0; i < childCount; ++i) {
			buildTree(&(list[i]), km.children[i].ID, km.children[i].moveU, 0);
		}
	}

	node->children.setChildren(list, childCount);
	node->restoreNode(Move(moveU), state, km.eval, km.mass);
}

void JosekiByKyokumen::output(SearchNode* root)
{
	if (!option.getC("joseki_by_kyokumen_on")) {
		return;
	}

	std::cout << "枝刈りを行います" << std::endl;
	double maxMass = root->mass;
	for (int i = 0; i < root->children.size(); ++i) {
		if (maxMass < root->children[i].mass) {
			maxMass = root->children[i].mass;
		}
	}
	std::vector<std::thread>th;
	for (int i = 0; i < root->children.size(); ++i) {
		th.push_back(std::thread(Pruning::pruningMass,&(root->children[i]), maxMass * option.getD("joseki_by_kyokumen_mass_rate")));
	}
	for (int i = 0; i < root->children.size(); ++i) {
		th[i].join();
	}
	std::cout << "枝刈り終了" << std::endl;

	StopWatch sw;
	sw.start();

	std::vector<std::string> usitokens = { "position","startpos" };
	Kyokumen kyokumen(usitokens);
	size_t rootID = 0;
	outputRecursive(0, kyokumen, root,&rootID,false);

	sw.print("outputRecurcive:");

	outputBinary();

	sw.print("outputBinary:");

	std::ofstream ofs("joseki\\node.txt");
	ofs << SearchNode::getNodeCount() << std::endl;
	ofs.close();
}

Move JosekiByKyokumen::getBestMove(std::vector<SearchNode*> history)
{
	Kyokumen kyokumen;
	Move best;
	for (int i = 1; i < history.size(); ++i) {
		kyokumen.proceed(history[i]->move);
	}
	kdkey key = kdkey(kyokumen.getBammen(), history.size() - 1);
	//for(int i = 0;)
	return false;
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
		if (itr->second.state == SearchNode::State::Expanded) {
			for (int i = 0; i < itr->second.children.size(); ++i) {
				msd.push_back(movesavedata(itr->second.ID, itr->second.children[i].moveU, itr->second.children[i].ID));
			}
		}
	}
	
	FILE* fp;
	if (fopen_s(&fp, FILENAME.c_str(), "wb") != 0) {
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

	if (option.getC("joseki_by_kyokumen_text_output_on")) {
		std::vector<std::string>outputs;
		outputs.resize(kyokumenCount);

		size_t nodeCount = 0;

		for (size_t i = 0; i < kyokumenCount; ++i) {
			outputs[ksd[i].ID] = "";
			outputs[ksd[i].ID] += std::to_string(ksd[i].ID) + "\t";

			outputs[ksd[i].ID] += std::to_string(kyokumenMap[keyList[ksd[i].ID]].count) + "\t";
			nodeCount += kyokumenMap[keyList[ksd[i].ID]].count;

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
		std::ofstream ofs(FILENAMETEXT);
		ofs << "nodeCount:" << nodeCount << std::endl;
		ofs << "kyokumenCount:" << kyokumenCount << std::endl;
		ofs << "childrenCount:" << childrenCount << std::endl;
		for (size_t i = 0; i < kyokumenCount; ++i) {
			ofs << outputs[i];
		}
		fclose(fp);
	}
}

void JosekiByKyokumen::outputRecursive(int hisCount, Kyokumen kyokumen, SearchNode* node, size_t* ID, bool multiThread)
{
	kyokumen.proceed(node->move);

	kdkey key(kyokumen.getBammen(), hisCount);

	bool duplicate, checkChildren = true;

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
			kd->state = node->getState();
		}
		else {
			//checkChildren = false;
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
	kd->count++;
	if (node->children.size() > 0) {
		if (kd->children.size() == 0) {
			kd->children.resize(node->children.size());
		}

		if (checkChildren) {
			if (false) {
				std::vector<std::thread>th;
				for (int i = 0; i < node->children.size(); ++i) {
					SearchNode* child = &(node->children[i]);
					th.push_back(std::thread(&JosekiByKyokumen::outputRecursive, this, hisCount + 1, kyokumen, child, &(kd->children[i].ID), false));
					kd->children[i].moveU = child->move.binary();
				}
				for (int i = 0; i < th.size(); ++i) {
					th[i].join();
				}
			}
			else {
				for (int i = 0; i < node->children.size(); ++i) {
					SearchNode* child = &(node->children[i]);
					outputRecursive(hisCount + 1, kyokumen, child, &(kd->children[i].ID), false);
					kd->children[i].moveU = child->move.binary();
				}
			}
		}
	}
}
