#pragma once
#include "josekioption.h"
#include "node.h"
#include "kyokumen.h"
#include <unordered_map>
#include <algorithm>

class JosekiByKyokumen {
public:
	JosekiByKyokumen();
	JosekiOption option;

	void input();
	void output(SearchNode* root);
private:
	void inputBinary();
	void outputBinary();
	void outputText();
	//https ://qiita.com/sokutou-metsu/items/6017a64b264ff023ec72

	//局面分解用
	struct kyokumendata {
		SearchNode* node;
		size_t kyokumenNumber;
		kyokumendata() {  };
		kyokumendata(size_t kn, Move move,SearchNode::State s, double eval, double mass) {
			kyokumenNumber = kn;
			node = new SearchNode();
			node->restoreNode(move, s, eval, mass);
		}
	};
	struct kdkey {
		Bammen bammen;
		int moveCount;
		
		kdkey() {};
		kdkey(Bammen b, int mc) {
			this->bammen = b;
			moveCount = mc;
		}

		bool operator==(const kdkey& rhs) const {
			const kdkey& lhs = *this;
			if (lhs.moveCount != rhs.moveCount) {
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
	//指し手の保存用
	struct mdkey {
		size_t parentKyokumenNumber;
		Move move;

		mdkey() {};
		mdkey(size_t pkn, Move m) {
			this->parentKyokumenNumber = pkn;
			this->move = m;
		}

		bool operator==(const mdkey& rhs) const {
			const mdkey& lhs = *this;
			return (lhs.parentKyokumenNumber == rhs.parentKyokumenNumber) && (lhs.move.binary() == rhs.move.binary());
		}

		bool operator!=(const mdkey& rhs) const {
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
		std::size_t operator()(const mdkey& key) const {
			std::string bytes(reinterpret_cast<const char*>(&key), sizeof(mdkey));
			return std::hash<std::string>()(bytes);
		}
	};

	//局面保存用
	struct kyokumensavedata {
		int moveCount;
		Bammen bammen;
		uint16_t moveU;
		double eval;
		double mass;
		int status;
		kyokumensavedata(int _moveCount, Bammen _bammen, double _eval, double _mass, SearchNode::State _state) {
			moveCount = _moveCount;
			bammen = _bammen;
			eval = _eval;
			mass = _mass;
			status = (int)_state;
		}
	};
	//指し手保存用
	struct movesavedata {
		size_t parentKyokumenNumber;
		uint16_t moveU;
		size_t childKyokmenNumber;
		movesavedata(size_t _parentKyokumenNumber, Move move, size_t _childKyokumenNumber) {
			parentKyokumenNumber = _parentKyokumenNumber;
			moveU = move.binary();
			childKyokmenNumber = _childKyokumenNumber;
		}
	};

	std::unordered_map<kdkey, kyokumendata, Hash> kyokumenMap;
	std::unordered_map<mdkey, size_t, Hash> moveMap;
	size_t outputRecursive(int moveCount, Kyokumen kyokumen, SearchNode* node);
};
