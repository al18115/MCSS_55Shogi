#include <iostream>
#include <fstream>
#include "josekibykyokumen.h"
#include "usi.h"
#include "pruning.h"

#define FILENAMEBASE (option.getS("joseki_by_kyokumen_folder") + "/" + option.getS("joseki_by_kyokumen_filename"))
#define FILENAME(add) (FILENAMEBASE + "_" +  add)
#define FILERENAME(add) (FILENAME(JosekiOption::getYMHM() + "_" + add))


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


template<typename T>
static bool freadVector(std::string filename, std::vector<T>& vec) {
	FILE* fp;
	size_t size;

	auto err = fopen_s(&fp, filename.c_str(), "rb");

	if (err) {
		std::cout << filename << " is not exist." << std::endl;
		return false;
	}

	std::cout << "input from " << filename << std::endl;

	fread(&size, sizeof(size_t), 1, fp);

	vec.resize(size);

	fread(&vec[0], sizeof(T), size, fp);

	fclose(fp);
}
template<typename T>
static void fwriteVector(std::string filename, std::vector<T> vec) {
	FILE* fp;
	size_t size;

	fopen_s(&fp, filename.c_str(), "wb");
	
	std::cout << "output for " << filename << std::endl;

	size = vec.size();
	
	fwrite(&size, sizeof(size_t), 1, fp);
	fwrite(&vec[0], sizeof(T), size, fp);
	
	fclose(fp);
}
template<typename T>
static void fwriteVectorText(std::string filename, std::vector<T> vec) {
	FILE* fp;
	size_t size;

	fopen_s(&fp, filename.c_str(), "w");

	std::cout << "output for " << filename << std::endl;

	size = vec.size();
	
	fwrite(&size, sizeof(size_t), 1, fp);
	fwrite(&vec[0], sizeof(T), size, fp);

	fclose(fp);
}



JosekiByKyokumen::JosekiByKyokumen(){
	option.addOption("joseki_by_kyokumen_on", "check", "false");
	option.addOption("joseki_by_kyokumen_folder", "string", "joseki");
	option.addOption("joseki_by_kyokumen_filename", "string", "jdk");
	option.addOption("joseki_by_kyokumen_text_output_on", "check", "false");
	option.addOption("joseki_by_kyokumen_multithread", "check", "false");
	option.addOption("joseki_by_kyokumen_mass_rate", "string", "0.5");
	option.addOption("joseki_by_kyokumen_rename", "check", "false");
	option.addOption("joseki_by_kyokumen_nodecountmax", "string", "1000");
}

static std::atomic<size_t> nodeCounter = 0;
static size_t nodeCountMax;
void JosekiByKyokumen::input(SearchTree *tree)
{

	bool inputOK = false;
	if (!option.getC("joseki_by_kyokumen_on")) {
		return;
	}

	std::cout << "start joseki input from " << FILENAME("node.bin") << std::endl;

	StopWatch sw;
	sw.start();
	inputOK = inputBinary();
	sw.print("inputBinaly:");
	SearchNode* root = new SearchNode;

	if (inputOK) {
		nodeCountMax = option.getI("joseki_by_kyokumen_nodecountmax");

		buildTree(root, 0, Move().binary(), option.getC("joseki_by_kyokumen_multithread"));

		sw.print("buildTree:");
		tree->setRoot(root);
	}

	std::cout << "node count : " << nodeCounter << std::endl;
}

bool JosekiByKyokumen::inputBinary()
{
	iodata io;

	//局面情報の読み込み
	if (freadVector(FILENAME("node.bin"), (io.kyokumens)) == false) {
		return false;
	}

	//指し手情報の読み込み
	freadVector(FILENAME("move.bin"), (io.moves));

	//IDの読み込み
	freadVector(FILENAME("id.bin"), (io.keys));

	//childpositionの読み込み
	freadVector(FILENAME("childposition.bin"), (io.childPosition));

	const size_t childrenCount = io.moves.size();
	const size_t kyokumenCount = io.kyokumens.size();
	std::vector<std::vector<kyokumendata::child>> kyokumenChildren;
	kyokumenChildren.resize(kyokumenCount);
	for (size_t i = 0; i < kyokumenCount; ++i) {
		const size_t childStart = io.childPosition[i].position;
		const size_t childCount = io.childPosition[i].count;
		for (size_t c = 0; c < childCount; ++c) {
			kyokumenChildren[i].push_back(kyokumendata::child(io.moves[childStart + c].moveU, io.moves[childStart + c].childKyokmenNumber));
		}
	}

	kyokumenDataArray = (kyokumendata*)malloc(sizeof(kyokumendata) * kyokumenCount);
	kyokumenMap.reserve(kyokumenCount);
	for (size_t i = 0; i < kyokumenCount; ++i) {
		const auto& ksdi = io.kyokumens[i];
		kdkey key = io.keys[i];
		auto& k = (kyokumenMap[i] = kyokumendata((SearchNode::State)ksdi.status, ksdi.eval, ksdi.mass));
		if (ksdi.status == (int)SearchNode::State::E) {
			k.children = kyokumenChildren[i];
		}
		idList[key] = i;
		if (kyokumenDataArray != NULL) {
			kyokumenDataArray[i] = k;
		}
		else {
			std::cout << "メモリの確保に失敗しました" << std::endl;
			exit(1);
		}
	}

	return true;
}

void JosekiByKyokumen::buildTree(SearchNode* node, size_t ID, uint16_t moveU, bool multiThread)
{
	++nodeCounter;

	const auto& km = kyokumenDataArray[ID];
	auto state = km.state;

	
	size_t childCount = km.children.size();
	SearchNode* list = new SearchNode[childCount];
	if (multiThread) {
		std::vector<std::thread>th;
		for (size_t i = 0; i < childCount; ++i) {
			th.push_back(std::thread(&JosekiByKyokumen::buildTree, this, &(list[i]), km.children[i].ID, km.children[i].moveU, false));
		}
		for (size_t i = 0; i < th.size(); ++i) {
			th[i].join();
		}
	}
	else {
		if (nodeCounter + childCount < nodeCountMax) {
			for (size_t i = 0; i < childCount; ++i) {
				buildTree(&(list[i]), km.children[i].ID, km.children[i].moveU, 0);
			}
		}
		else {
			if (state == SearchNode::State::E) {
				state = SearchNode::State::NotExpanded;
			}
			childCount = 0;
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
	
	std::cout << "start output" << std::endl;
	outputRecursive(0, kyokumen, root,rootID,false);

	renameOutputFile();

	sw.print("outputRecurcive:");

	outputBinary();

	sw.print("outputBinary:");
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
	iodata io;

	io.set(kyokumenMap, idList);

	//ノードの保存
	fwriteVector(FILENAME("node.bin"), io.kyokumens);

	//ムーブの保存
	fwriteVector(FILENAME("move.bin"), io.moves);

	//IDの保存
	fwriteVector(FILENAME("id.bin"), io.keys);

	//childpositionの保存
	fwriteVector(FILENAME("childposition.bin"), io.childPosition);


	//テキストで書き出し
	if (option.getC("joseki_by_kyokumen_text_output_on")) {
		//ノードの保存
		{
			std::ofstream ofs(FILENAME("node.txt"));
			if (ofs.is_open()) {
				std::cout << "output to " << FILENAME("node.txt") << std::endl;
				const auto& target = io.kyokumens;
				ofs << target.size() << std::endl;
				for (const auto& t : target) {
					ofs << t.status << "\t";
					ofs << t.eval << "\t";
					ofs << t.mass << "\t";
					ofs << std::endl;
				}
			}
			ofs.close();
		}


		//ムーブの保存
		{
			std::ofstream ofs(FILENAME("move.txt"));
			if (ofs.is_open()) {
				std::cout << "output to " << FILENAME("move.txt") << std::endl;
				const auto& target = io.moves;
				ofs << target.size() << std::endl;
				for (const auto& t : target) {
					ofs << t.moveU << "\t";
					ofs << t.childKyokmenNumber << "\t";
					ofs << std::endl;
				}
			}
			ofs.close();
		}


		//IDの保存
		{
			std::ofstream ofs(FILENAME("id.txt"));
			if (ofs.is_open()) {
				std::cout << "output to " << FILENAME("id.txt") << std::endl;
				const auto& target = io.keys;
				ofs << target.size() << std::endl;
				for (const auto& t : target) {
					ofs << t.hisCount << "\t";
					ofs << Kyokumen(t.bammen, true).toSfen() << "\t";
					ofs << std::endl;
				}
			}
			ofs.close();
		}

		//childpositionの保存
		{
			std::ofstream ofs(FILENAME("childposition.txt"));
			if (ofs.is_open()) {
				std::cout << "output to " << FILENAME("childposition.txt") << std::endl;
				const auto& target = io.childPosition;
				ofs << target.size() << std::endl;
				for (const auto& t : target) {
					ofs << t.position << "\t";
					ofs << t.count << "\t";
					ofs << std::endl;
				}
			}
			ofs.close();
		}

	}
}

void JosekiByKyokumen::renameOutputFile(){
	if (option.getC("joseki_by_kyokumen_rename")) {
		std::vector<std::string> v = {
			"node",
			"move",
			"id",
			"childposition",
		};
		for (int i = 0; i < v.size(); ++i) {
			std::string name = v[i] + ".bin";
			std::rename(FILENAME(name).c_str(), FILERENAME(name).c_str());
		}
		if (option.getC("joseki_by_kyokumen_text_output_on")) {
			for (int i = 0; i < v.size(); ++i) {
				std::string name = v[i] + ".txt";
				std::rename(FILENAME(name).c_str(), FILERENAME(name).c_str());
			}
		}
	}
}

std::vector<StopWatch> swa;

void JosekiByKyokumen::outputRecursive(int hisCount, Kyokumen kyokumen, SearchNode* node, size_t& ID, bool multiThread)
{
	if (hisCount == 0) {
		for (int i = 0; i < 3; ++i) {
			swa.push_back(StopWatch());
		}
	}

	swa[0].start();
	kyokumen.proceed(node->move);
	swa[0].stop();

	swa[1].start();
	ID = getID(hisCount, kyokumen.getBammen());
	swa[1].stop();

	bool duplicate, checkChildren = true;

	swa[2].start();
	auto itr = kyokumenMap.find(ID);
	duplicate = (itr != kyokumenMap.end());
	swa[2].stop();

	kyokumendata* kd;
	if (duplicate) {
		//既に局面が保存されている
		kd = &kyokumenMap[ID];
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
		kyokumendata newkd(node->getState(), node->eval, node->mass);
		kyokumenMap[ID] = newkd;
		kd = &kyokumenMap[ID];
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
					th.push_back(std::thread(&JosekiByKyokumen::outputRecursive, this, hisCount + 1, kyokumen, child, std::ref(kd->children[i].ID), false));
					kd->children[i].moveU = child->move.binary();
				}
				for (int i = 0; i < th.size(); ++i) {
					th[i].join();
				}
			}
			else {
				for (int i = 0; i < node->children.size(); ++i) {
					SearchNode* child = &(node->children[i]);
					outputRecursive(hisCount + 1, kyokumen, child, (kd->children[i].ID), false);
					kd->children[i].moveU = child->move.binary();
				}
			}
		}
	}

	if (hisCount == 0) {
		for (int i = 0; i < swa.size(); ++i) {
			swa[i].print("sw" + std::to_string(i) + ":");
		}
	}
}

inline size_t JosekiByKyokumen::getID(const int hisCount,const Bammen bammen)
{
	size_t ID;
	const auto key = kdkey(bammen, hisCount);
	auto itr = idList.find(key);
	if (itr == idList.end()) {
		ID = idList.size();
		idList[key] = ID;
	}
	else {
		ID = idList[key];
	}
	return ID;
}

void JosekiByKyokumen::iodata::set(std::unordered_map<size_t, kyokumendata>& kyokumenMap, std::unordered_map<kdkey, size_t, Hash>& idList){
	keys.resize(idList.size());
	kyokumens.resize(idList.size());
	std::vector<std::vector<kyokumendata::child>>childrens(idList.size());
	size_t childCount = 0, position = 0;

	for (auto itr = idList.begin(); itr != idList.end(); ++itr) {
		auto id = itr->second;
		auto& dist = kyokumens[id];
		auto& source = kyokumenMap[id];
		keys[id] = itr->first;
		dist.eval = source.eval;
		dist.mass = source.mass;
		dist.status = (int)source.state;
		childrens[id] = source.children;
		childCount += source.children.size();
	}

	moves.reserve(childCount);
	childPosition.reserve(childrens.size());
	for (size_t i = 0; i < childrens.size(); ++i) {
		auto& target = childrens[i];
		for (size_t j = 0; j < target.size(); ++j) {
			moves.push_back(movesavedata(target[j].moveU, target[j].ID));
		}
		child c;
		c.count = target.size();
		c.position = position;
		childPosition.push_back(c);
		position += target.size();
	}
}
