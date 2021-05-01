#include <iostream>
#include <fstream>
#include "josekibykyokumen.h"

JosekiByKyokumen::JosekiByKyokumen(){
	option.addOption("joseki_by_kyokumen_on", "check", "true");
}

void JosekiByKyokumen::input(SearchTree *tree)
{
	inputBinary();
	kyokumendata* kdv = new kyokumendata[kyokumenMap.size()];
	for (auto itr = kyokumenMap.begin(); itr != kyokumenMap.end(); ++itr) {
		kdv[itr->second.ID] = itr->second;
	}

	SearchNode* root = new SearchNode;
	buildTree(root,0, kdv);

	tree->setRoot(root);
	delete[] kdv;
}

void JosekiByKyokumen::output(SearchNode* root)
{
	std::vector<std::string> usitokens = {"position","startpos"};
	Kyokumen kyokumen(usitokens);
	outputRecursive(0, kyokumen, root);

	outputBinary();
}

void JosekiByKyokumen::inputBinary()
{
	FILE* fp;
	if (fopen_s(&fp, "joseki/testKyokumen.bin", "rb") != 0) {
		std::cout << "file open error." << std::endl;
		return;
	}

	size_t kyokumenCount;
	fread(&kyokumenCount, sizeof(size_t), 1, fp);
	std::vector<kyokumensavedata>ksd;
	ksd.resize(kyokumenCount);
	fread(&ksd[0], sizeof(kyokumensavedata), kyokumenCount, fp);

	size_t childrenCount;
	fread(&childrenCount, sizeof(size_t), 1, fp);
	std::vector<movesavedata> msd;
	msd.resize(childrenCount);
	fread(&msd[0], sizeof(movesavedata), childrenCount, fp);

	fclose(fp);

	std::unordered_map<size_t,std::vector<size_t>> kyokumenChildren;
	for (int i = 0; i < childrenCount; ++i) {
		kyokumenChildren[msd[i].parentKyokumenNumber].push_back(msd[i].childKyokmenNumber);
	}

	//for (size_t i = 0; i < kyokumenCount; ++i) {
	//	kyokumenMap[kdkey(ksd[i].bammen, ksd[i].moveCount)] = kyokumendata(ksd[i].ID, Move(ksd[i].moveU), (SearchNode::State)ksd[i].status, ksd[i].eval, ksd[i].mass);
	//	kyokumenMap[kdkey(ksd[i].bammen, ksd[i].moveCount)].childrenID = kyokumenChildren[ksd[i].ID];
	//}
	for (size_t i = 0; i < kyokumenCount; ++i) {
		size_t id = ksd[i].ID;
		Move move = Move(ksd[i].moveU);
		SearchNode::State state = (SearchNode::State)ksd[i].status;
		double eval = ksd[i].eval;
		double mass = ksd[i].mass;
		Bammen bammen = ksd[i].bammen;
		int moveCount = ksd[i].moveCount;
		kdkey key(bammen, moveCount);
		kyokumenMap[key] = kyokumendata(id, move, state, eval, mass);
		kyokumenMap[key].childrenID = kyokumenChildren[ksd[i].ID];

		//std::cout << id << std::endl;
	}
}

SearchNode* JosekiByKyokumen::buildTree(SearchNode* node, size_t ID,kyokumendata* kdv)
{
	auto km = kdv[ID];
	node->restoreNode(km.node->move, km.node->getState(), km.node->eval, km.node->mass);
	size_t childCount = km.childrenID.size();
	SearchNode* list = new SearchNode[childCount];
	for (int i = 0; i < childCount; ++i) {
		buildTree(&(list[i]), km.childrenID[i], kdv);
	}
	node->children.setChildren(list,childCount);
	std::cout << ID << std::endl;
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
				itr->second.node->move.binary(),
				itr->second.node->eval,
				itr->second.node->mass,
				itr->second.node->getState()
			)
		);
		for (int i = 0; i < itr->second.childrenID.size();++i) {
			msd.push_back(movesavedata(itr->second.ID, itr->second.childrenID[i]));
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
		std::ofstream ofs("joseki/testkyokumen.txt");

		ofs << kyokumenCount << std::endl;

		for (size_t i = 0; i < kyokumenCount; ++i) {
			ofs << ksd[i].ID << "\t";
			ofs << Kyokumen(ksd[i].bammen, true).toSfen() << "\t";
			ofs << ksd[i].moveCount << "\t";
			ofs << ksd[i].status << "\t";
			ofs << ksd[i].eval << "\t";
			ofs << ksd[i].mass << "\t";
			ofs << ksd[i].moveU << std::endl;
		}

		ofs << childrenCount << std::endl;

		for (size_t i = 0; i < childrenCount; ++i) {
			ofs << msd[i].parentKyokumenNumber << "\t" << msd[i].childKyokmenNumber << std::endl;
		}

		fclose(fp);
	}
}

size_t JosekiByKyokumen::outputRecursive(int hisCount, Kyokumen kyokumen, SearchNode* node)
{
	size_t ID;

	kdkey kk(kyokumen.getBammen(), hisCount);
	
	auto itr = kyokumenMap.find(kk);
	if (itr != kyokumenMap.end()) {
		if (itr->second.node->mass < node->mass) {
			itr->second.node = node;
			ID = itr->second.ID;
		}
	}
	else {
		ID = kyokumenMap.size();
		kyokumendata kd(ID, node);
		kyokumenMap[kk] = kd;
		size_t last = -1;
		for (int i = 0; i < node->children.size(); ++i) {
			Kyokumen childKyokumen = kyokumen;
			childKyokumen.proceed(node->children[i].move);
			size_t childID = outputRecursive(hisCount + 1, childKyokumen, &node->children[i]);

			kyokumenMap[kk].childrenID.push_back(childID);
		}
	}
	return ID;
}
