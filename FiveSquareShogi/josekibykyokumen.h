#pragma once
#include "josekioption.h"
#include "node.h"
#include "kyokumen.h"
#include "tree.h"
#include <unordered_map>
#include <algorithm>

class JosekiByKyokumen {
public:
	JosekiByKyokumen();
	JosekiOption option;

	void input(SearchTree* tree);
	void output(SearchNode* root);
	Move getBestMove(std::vector<SearchNode*>history);
private:
	//https ://qiita.com/sokutou-metsu/items/6017a64b264ff023ec72

	//局面分解用
	struct kyokumendata {
		double eval;
		double mass;
		SearchNode::State state;
		int count = 0;
		struct child {
			uint16_t moveU;
			size_t ID;
			child():moveU(-1),ID(-1) {}
			child(uint16_t _moveU, size_t _ID) :moveU(_moveU), ID(_ID) {}
		};
		std::vector<child>children;
		kyokumendata() :kyokumendata(SearchNode::State::N,-1,-1) {}
		kyokumendata(SearchNode::State _state, double _eval, double _mass) :eval(_eval), mass(_mass), state(_state) { count = 0; }
	};
	struct kdkey {
		Bammen bammen;
		int hisCount;
		
		kdkey() :kdkey({ 0 },-1) {}
		kdkey(Bammen _bammen, int _hisCount): bammen(_bammen), hisCount(_hisCount) {}
		bool operator==(const kdkey& rhs) const {
			const kdkey& lhs = *this;
			return (lhs.hisCount == rhs.hisCount) && (lhs.bammen == rhs.bammen);
		}

		bool operator!=(const kdkey& rhs) const {
			return !(this->operator==(rhs));
		}
	};
	//ハッシュ関係
	struct Hash {
		std::size_t operator()(const kdkey& key) const {
			size_t h = key.hisCount;
			for (int i = 0; i < 35; ++i) {
				h += (size_t)key.bammen[i] * (3 * (size_t)i + 140);
			}
			/*uint64_t h =
				((static_cast<uint64_t>(key.hisCount) & 1) << 25)
				^ ((static_cast<uint64_t>(key.bammen[0]) & 1) << 26)
				^ ((static_cast<uint64_t>(key.bammen[1]) & 1) << 27)
				^ ((static_cast<uint64_t>(key.bammen[2]) & 1) << 28)
				^ ((static_cast<uint64_t>(key.bammen[3]) & 1) << 29)
				^ ((static_cast<uint64_t>(key.bammen[4]) & 1) << 30)
				^ ((static_cast<uint64_t>(key.bammen[5]) & 1) << 31)
				^ ((static_cast<uint64_t>(key.bammen[6]) & 1) << 32)
				^ ((static_cast<uint64_t>(key.bammen[7]) & 1) << 33)
				^ ((static_cast<uint64_t>(key.bammen[8]) & 1) << 34)
				^ ((static_cast<uint64_t>(key.bammen[9]) & 1) << 35)
				^ ((static_cast<uint64_t>(key.bammen[10]) & 1) << 36)
				^ ((static_cast<uint64_t>(key.bammen[11]) & 1) << 37)
				^ ((static_cast<uint64_t>(key.bammen[12]) & 1) << 38)
				^ ((static_cast<uint64_t>(key.bammen[13]) & 1) << 39)
				^ ((static_cast<uint64_t>(key.bammen[14]) & 1) << 40)
				^ ((static_cast<uint64_t>(key.bammen[16]) & 1) << 41)
				^ ((static_cast<uint64_t>(key.bammen[17]) & 1) << 42)
				^ ((static_cast<uint64_t>(key.bammen[18]) & 1) << 43)
				^ ((static_cast<uint64_t>(key.bammen[19]) & 1) << 44)
				^ ((static_cast<uint64_t>(key.bammen[20]) & 1) << 45)
				^ ((static_cast<uint64_t>(key.bammen[21]) & 1) << 46)
				^ ((static_cast<uint64_t>(key.bammen[22]) & 1) << 47)
				^ ((static_cast<uint64_t>(key.bammen[23]) & 1) << 48)
				^ ((static_cast<uint64_t>(key.bammen[24]) & 1) << 49)
				^ ((static_cast<uint64_t>(key.bammen[25]) & 1) << 50)
				^ ((static_cast<uint64_t>(key.bammen[26]) & 1) << 51)
				^ ((static_cast<uint64_t>(key.bammen[27]) & 1) << 52)
				^ ((static_cast<uint64_t>(key.bammen[28]) & 1) << 53)
				^ ((static_cast<uint64_t>(key.bammen[29]) & 1) << 54)
				^ ((static_cast<uint64_t>(key.bammen[30]) & 1) << 55)
				^ ((static_cast<uint64_t>(key.bammen[31]) & 1) << 56)
				^ ((static_cast<uint64_t>(key.bammen[32]) & 1) << 57)
				^ ((static_cast<uint64_t>(key.bammen[33]) & 1) << 58)
				^ ((static_cast<uint64_t>(key.bammen[34]) & 1) << 59);
			*/
			return h;
		}
	};

	//局面保存用
	struct kyokumensavedata {
		double eval;
		double mass;
		int status;
		kyokumensavedata() :kyokumensavedata(-1,-1,SearchNode::State::N) {}
		kyokumensavedata(double _eval, double _mass, SearchNode::State _state) :eval(_eval), mass(_mass), status((int)_state) {}
	};
	//指し手保存用
	struct movesavedata{
		uint16_t moveU;
		size_t childKyokmenNumber;
		movesavedata():movesavedata(-1,-1) {}
		movesavedata(uint16_t _moveU, size_t _childKyokumenNumber):moveU(_moveU),childKyokmenNumber(_childKyokumenNumber) {}
	};
	//読み書き時に値を一時保存する用
	struct iodata {
		struct child { size_t position; size_t count; };
		std::vector<kdkey> keys;
		std::vector<kyokumensavedata> kyokumens;
		std::vector<movesavedata> moves;
		std::vector<child> childPosition;

		void set(std::unordered_map<size_t, kyokumendata>& kyokumenMap, std::unordered_map<kdkey, size_t, Hash>& idList);
	};

	bool inputBinary();
	void buildTree(SearchNode* node, size_t ID, uint16_t moveU,bool multiThread);
	void outputBinary();
	//書き出し直前に使用。上書きを避けるため、ファイル名を現在時刻に依存するものに書き換える。
	void renameOutputFile();
	std::unordered_map<size_t, kyokumendata> kyokumenMap;
	std::unordered_map<kdkey, size_t, Hash> idList;
	kyokumendata* kyokumenDataArray;
	void outputRecursive(int hisCount, Kyokumen kyokumen, SearchNode* node,size_t &childID,bool multiThread);
	size_t getID(const int hisCount,const Bammen bammen);
};
