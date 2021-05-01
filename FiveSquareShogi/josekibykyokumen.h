#pragma once
#include "josekioption.h"
#include "node.h"
#include "kyokumen.h"
#include "tree.h"
#include <unordered_map>
#include <algorithm>
#include <set>

class JosekiByKyokumen {
public:
	JosekiByKyokumen();
	JosekiOption option;

	void input(SearchTree* tree);
	void output(SearchNode* root);
private:
	//https ://qiita.com/sokutou-metsu/items/6017a64b264ff023ec72

	//局面分解用
	struct kyokumendata {
		SearchNode* node;
		size_t ID;
		std::vector<size_t>childrenID;
		kyokumendata() {}
		kyokumendata(size_t _ID, SearchNode* _node) {
			ID = _ID;
			node = _node;
		}
		kyokumendata(size_t _ID, Move move,SearchNode::State state, double eval, double mass) {
			ID = _ID;
			node = new SearchNode;
			node->restoreNode(move, state, eval, mass);
		}
	};
	struct kdkey {
		Bammen bammen;
		int hisCount;
		
		kdkey() {}
		kdkey(Bammen _bammen, int _hisCount) {
			this->bammen = _bammen;
			hisCount = _hisCount;
			return;
		}

		//kdkey operator=(const kdkey& rhs){
		//	bammen = rhs.bammen;
		//	hisCount = rhs.hisCount;
		//	return *this;
		//}

		bool operator==(const kdkey& rhs) const {
			const kdkey& lhs = *this;
			if (lhs.hisCount != rhs.hisCount) {
				return false;
			}
			for (int i = 0; i < lhs.bammen.size(); ++i) {
				if (lhs.bammen[i] != rhs.bammen[i]) {
					return false;
				}
			}
			return true;
		}

		bool operator!=(const kdkey& rhs) const {
			return !(this->operator==(rhs));
		}
	};
	//ハッシュ関係
	struct Hash {
		typedef std::size_t result_type;

		std::size_t operator()(const kdkey& key) const {
			std::string bytes(reinterpret_cast<const char*>(&key), sizeof(kdkey));
			return std::hash<std::string>()(bytes);
		}
	};

	//局面保存用
	struct kyokumensavedata {
		size_t ID;
		int moveCount;
		Bammen bammen;
		uint16_t moveU;
		double eval;
		double mass;
		int status;
		kyokumensavedata(){}
		kyokumensavedata(size_t _ID,int _moveCount, Bammen _bammen,uint16_t _moveU, double _eval, double _mass, SearchNode::State _state) {
			ID = _ID;
			moveCount = _moveCount;
			moveU = _moveU;
			bammen = _bammen;
			eval = _eval;
			mass = _mass;
			status = (int)_state;
		}
	};
	//指し手保存用
	struct movesavedata {
		size_t parentKyokumenNumber;
		size_t childKyokmenNumber;
		movesavedata() {}
		movesavedata(size_t _parentKyokumenNumber, size_t _childKyokumenNumber) {
			parentKyokumenNumber = _parentKyokumenNumber;
			childKyokmenNumber = _childKyokumenNumber;
		}
	};

	void inputBinary();
	SearchNode* buildTree(SearchNode* node, size_t ID, kyokumendata* kdv);
	void outputBinary();

	std::unordered_map<kdkey, kyokumendata, Hash> kyokumenMap;
	size_t outputRecursive(int hisCount, Kyokumen kyokumen, SearchNode* node);
};
