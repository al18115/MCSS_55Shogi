#include <iostream>
#include <fstream>
#include "josekibykyokumen.h"
#include "usi.h"
#include "pruning.h"
#include "MMapVector.h"
#include "agent.h"

#define FILENAMEBASE (option.getS("joseki_by_kyokumen_folder") + "/" + option.getS("joseki_by_kyokumen_filename"))
#define FILENAME(add) (FILENAMEBASE + "_" +  add)
#define FILERENAME(add) (FILENAME(JosekiOption::getYMHM() + "_" + add))
#define CHECKJOSEKIISON {if(!option.getC("joseki_by_kyokumen_on"))return;}

//https ://qiita.com/sokutou-metsu/items/6017a64b264ff023ec72
//ハッシュ関係
struct Hash {
	std::size_t operator()(const JosekiByKyokumen::kdkey& key) const {
		//ハッシュ値の計算方法：手数カウントと盤面の各マスに素数を掛けて総和をとる。
		constexpr size_t prime[] = {
			3313,3319,3323,3329,3331,
			3343,3347,3359,3361,3371,
			3373,3389,3391,3407,3413,
			3433,3449,3457,3461,3463,
			3467,3469,3491,3499,3511,
			3517,3527,3529,3533,3539,
			3541,3547,3557,3559,3571
		};
		size_t h = (size_t)key.hisCount * 3307;
		for (int i = 0; i < 35; ++i) {
			h += (size_t)key.bammen[i] * (prime[i]);
		}
		return h;
	}
};
//局面保存用
struct kyokumensavedata {
	double eval;
	double mass;
	int status;
	kyokumensavedata() :kyokumensavedata(-1, -1, SearchNode::State::N) {}
	kyokumensavedata(double _eval, double _mass, SearchNode::State _state) :eval(_eval), mass(_mass), status((int)_state) {}
};
//指し手保存用
struct movesavedata {
	uint16_t moveU;
	size_t childKyokmenNumber;
	movesavedata() :movesavedata(-1, -1) {}
	movesavedata(uint16_t _moveU, size_t _childKyokumenNumber) :moveU(_moveU), childKyokmenNumber(_childKyokumenNumber) {}
};
//指し手の保存位置保存用
struct childposition {
	size_t position;
	size_t count;
};

//初期局面での手数カウント
const int firstHisCount = 1;
//手数カウントを受け取り、先手かどうかを返す
bool isSente(const int hisCount) { return (hisCount % 2) == firstHisCount; };

//探索木からもらったノードなどの各種データを、入出力するクラス
class IOJoseki {
public:
	//4つのファイル名を受け取り初期化
	inline void init(std::string filenameNode, std::string filenameID, std::string filenameChildPosition, std::string filenameMoves) {
		if (isOpen) {
			return;
		}
		isOpen = true;
		std::cout << "filenameNode:" << filenameNode << std::endl;
		mv.node.init(filenameNode);
		mv.childPosition.init(filenameChildPosition);
		mv.moves.init(filenameMoves);
		mv.id.init(filenameID);
		//全局面IDの取得
		loadIDFromFile();
	}
	//指定されたキーのIDを取得し、ノードを格納
	inline void set(const JosekiByKyokumen::kdkey key, const SearchNode* node) {
		//ファイルを書き換えるかどうか
		bool updateF = false;
		kyokumensavedata ksd;
		//IDの取得
		auto ID = getID(key);
		//保存済みのノードをロード
		mv.node.read(ID, ksd);
		//保存されていた格納状態を取得しておく
		auto state = (SearchNode::State)ksd.status;

		//保存されていたノードより深ければ格納
		if (ksd.mass < node->mass || (int)node->mass == 0) {
			ksd.eval = node->eval;
			ksd.mass = node->mass;
			if (!node->isLeaf()) {
				ksd.status = (int)node->getState();
			}
			updateF = true;
		}

		//定跡に子ノードのIDが格納されていなければ格納
		if (state == SearchNode::State::NotExpanded && node->getState() == SearchNode::State::Expanded) {
			childposition c;
			const auto& children = node->children;
			//子の保存の最初の位置を設定
			c.position = mv.moves.getCount();
			//子の数を保存
			c.count = children.size();

			//子の保存のために、親局面と子の手数カウントを保存
			//const auto parentKyokumen = Kyokumen(key.bammen, isSente(key.hisCount));
			const SearchPlayer parentPlayer(Kyokumen(key.bammen, isSente(key.hisCount)));
			const auto nextHisCount = key.hisCount + 1;
			for (int i = 0; i < c.count; ++i) {
				const auto& child = children[i];
				SearchPlayer player = parentPlayer;
				//子の指し手で局面を進める
				player.proceed(child.move);
				//進めた局面でIDを取得
				size_t childID = getID(JosekiByKyokumen::kdkey(player.kyokumen.getBammen(), nextHisCount));

				//子のIDをファイルに保存
				mv.moves.write(c.position + i, movesavedata(child.move.binary(), childID));
			}
			//子の保存位置と数を保存
			mv.childPosition.write(ID, c);

			updateF = true;
		}

		//定跡に上書きが必要なら上書きする
		if (updateF) {
			mv.node.write(ID, ksd);
		}
	}
	//ノードに値を格納する
	inline void getNode(size_t ID, SearchNode* node, Move move) {
		kyokumensavedata ksd;
		//ノードデータの読み込み
		mv.node.read(ID, ksd);
		if (move.binary() == Move().binary()) {
			move = node->move;
		}
		//ノードのほうが値が大きければノードのデータは変更しない
		if (node->mass >= ksd.mass) {
			ksd.mass = node->mass;
			ksd.eval = node->eval;
			ksd.status = (int)node->getState();
		}
		node->restoreNode(move, (SearchNode::State)ksd.status, ksd.eval, ksd.mass);
	}
	//ノードの子ノードのIDを取得する
	inline std::vector<movesavedata> getChildren(const size_t ID) {
		childposition c;
		std::vector<movesavedata>children;
		//子ノード情報が保存されている位置を取得
		mv.childPosition.read(ID, c);
		children.resize(c.count);
		//格納された位置に順に子ノードのID並んでいるので、それを取得
		for (int i = 0; i < c.count; ++i) {
			mv.moves.read(c.position + i, children[i]);
		}
		return children;
	}
	//historyから現在の局面を特定し、そのIDを返す。
	inline size_t getKyokumenID(const std::vector<SearchNode*>& history) {
		Kyokumen kyokumen = Kyokumen();
		int hisCount = firstHisCount;
		for (int i = 1; i < history.size(); ++i) {
			kyokumen.proceed(history[i]->move);
			++hisCount;
		}
		return getID(JosekiByKyokumen::kdkey(kyokumen.getBammen(), hisCount));
	}
	//ノードの情報を出力
	inline void printKyokumen(size_t ID) {
		kyokumensavedata ksd;
		childposition c;
		JosekiByKyokumen::kdkey key;
		movesavedata msd;
		mv.node.read(ID, ksd);
		mv.id.read(ID, key);
		mv.childPosition.read(ID, c);

		std::cout << "ID:" << ID << std::endl;
		std::cout << "sfen:" << Kyokumen(key.bammen, isSente(key.hisCount)).toSfen() << std::endl;
		std::cout << "hisCount:" << key.hisCount << std::endl;
		std::cout << "eval:" << ksd.eval << std::endl;
		std::cout << "mass:" << ksd.mass << std::endl;
		std::cout << "status:" << ksd.status << std::endl;
		std::cout << ":children:" << std::endl;

		for (int i = 0; i < c.count; ++i) {
			mv.moves.read(c.position + i, msd);
			std::cout << "ID:" << msd.childKyokmenNumber << "\t" << "move:" << msd.moveU << "(" << Move(msd.moveU).toUSI() << ")" << std::endl;
		}
	}

	void fin() {
		mv.fin();
		isOpen = false;
	}
private:
	bool isOpen = false;
	//ファイル入出力クラス集
	struct MMapVectors {
		MMapVector<kyokumensavedata> node;
		MMapVector<JosekiByKyokumen::kdkey> id;
		MMapVector<movesavedata>moves;
		MMapVector<childposition>childPosition;
		void fin() {
			node.fin();
			id.fin();
			moves.fin();
			childPosition.fin();
		}
	}mv;
	//局面集を保存しておくunordered_map。最初にすべて読み込んでおく
	std::unordered_map<JosekiByKyokumen::kdkey, size_t, Hash>keys;
	//ファイルからIDを全部読み込み
	void loadIDFromFile() {
		if (!keys.empty()) {
			return;
		}
		size_t count = mv.id.getCount();
		JosekiByKyokumen::kdkey* keyData = new JosekiByKyokumen::kdkey[count];
		mv.id.allLoad((void*)(keyData));
		int tempSize = keys.size();
		for (size_t i = 0; i < count; ++i) {
			keys[keyData[i]] = i;
			if (keys.size() == tempSize) {
				std::cout << Kyokumen(keyData[i].bammen, isSente(keyData[i].hisCount)).toSfen() << std::endl;
				int n = 0;
			}
			tempSize = keys.size();
		}
		std::cout << "key load ok:" << count << std::endl;
		delete[] keyData;
	}
	//keyからIDを取得。keyが今まで登録されていなければ、IDを新たに付与する。
	size_t getID(const JosekiByKyokumen::kdkey key)
	{
		size_t ID;
		auto itr = keys.find(key);
		if (itr == keys.end()) {
			ID = keys.size();
			keys[key] = ID;
			mv.id.write(ID, key);
		}
		else {
			ID = keys[key];
		}
		return ID;
	}
}iojoseki;

//時間計測用のクラス
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

//オプション
JosekiByKyokumen::JosekiByKyokumen() {
	option.addOption("joseki_by_kyokumen_on", "check", "false");
	option.addOption("joseki_by_kyokumen_folder", "string", "joseki");
	option.addOption("joseki_by_kyokumen_filename", "string", "jdk");
	option.addOption("joseki_by_kyokumen_text_output_on", "check", "false");
	option.addOption("joseki_by_kyokumen_multithread", "check", "false");
	option.addOption("joseki_by_kyokumen_mass_rate", "string", "0.5");
	option.addOption("joseki_by_kyokumen_rename", "check", "false");
	option.addOption("joseki_by_kyokumen_nodecountmax", "string", "1000");
	option.addOption("joseki_by_kyokumen_borderHisCount", "string", "4");
}

//初期化
void JosekiByKyokumen::init() {
	if (!option.getC("joseki_by_kyokumen_on")) {
		return;
	}
	iojoseki.init(FILENAME("queue_node.bin"), FILENAME("queue_id.bin"), FILENAME("queue_childposition.bin"), FILENAME("queue_move.bin"));
}

void JosekiByKyokumen::outputQueue(SearchTree& tree) {
	CHECKJOSEKIISON;

	//historyがなければ出力するものもないので終了
	if (tree.getHistory().empty()) {
		std::cout << "history is empty." << std::endl;
		return;
	}

	std::cout << "OutputQueue Start" << std::endl;
	//定跡ファイルが開かれてなかったら開く
	iojoseki.init(FILENAME("queue_node.bin"), FILENAME("queue_id.bin"), FILENAME("queue_childposition.bin"), FILENAME("queue_move.bin"));

	//ノード数をカウントする
	size_t nodeCount = 0;
	//手数カウントを取得
	auto firstKyokumenHisCount = tree.getHistory().size();
	//保存するノードの深さの上限を設定する
	int borderHis = firstKyokumenHisCount + option.getI("joseki_by_kyokumen_borderHisCount");
	//trueの間のみキューにノードを追加する
	bool add = true;

	//出力用のキュー
	std::queue<std::pair<kdkey, SearchNode*>> q;
	//ゲームそのものの初期局面を取得
	Kyokumen startkyokumen;
	//初期局面でのノードを取得
	SearchNode* root = tree.getHistory().front();
	//historyの末端まで局面を進める
	for (int i = 1; i < tree.getHistory().size(); ++i) {
		root = (tree.getHistory())[i];
		startkyokumen.proceed(root->move);
	}

	//初期局面をキューに追加
	q.push(std::make_pair(kdkey(startkyokumen.getBammen(), firstKyokumenHisCount), root));
	while (!q.empty()) {
		//キューの先頭を取り出す
		const auto front = q.front();
		const auto key = front.first;
		auto node = front.second;
		q.pop();

		//深さの上限に達していたら追加しない
		if (add && borderHis <= key.hisCount) {
			add = false;
		}

		//子ノードがあればキューに追加する
		if (add && (node->getState() == SearchNode::State::E)) {
			//const Kyokumen parentKyokumen = Kyokumen(key.bammen, isSente(key.hisCount));
			const SearchPlayer parentPlayer(Kyokumen(key.bammen, isSente(key.hisCount)));
			const int nextHisCount = key.hisCount + 1;
			for (int i = 0; i < node->children.size(); ++i) {
				SearchPlayer player = parentPlayer;
				player.proceed(node->children[i].move);

				q.push(std::make_pair(kdkey(player.kyokumen.getBammen(), nextHisCount), &node->children[i]));
			}
		}
		else {
			//ノードの展開状態が展開中か未展開なら未展開にする
			if (node->isLeaf()) {
				node->setState(SearchNode::State::N);
			}
		}

		//ノードを出力
		iojoseki.set(key, node);

		nodeCount++;
		if (nodeCount % 1000000 == 0) {
			//ノード数に応じて定期的に情報を出力
			std::cout << "qsize : " << q.size() << "\tnodeCount : " << nodeCount << std::endl;
		}
	}
	iojoseki.fin();
}

void JosekiByKyokumen::inputQueue(SearchTree& tree) {
	CHECKJOSEKIISON;

	std::cout << "InputQueue Start" << std::endl;

	//定跡ファイルが開かれてなかったら開く
	iojoseki.init(FILENAME("queue_node.bin"), FILENAME("queue_id.bin"), FILENAME("queue_childposition.bin"), FILENAME("queue_move.bin"));

	//出力したノード数カウント
	size_t nodeCount = 0;
	//キューに格納する用の構造体
	struct qval {
		SearchNode* node = nullptr;
		size_t ID = 0;
		Move move = Move();
		int hisCount = -1;
		qval(SearchNode* node, size_t id, int hisCount, uint16_t move = Move().binary()) {
			this->node = node;
			this->ID = id;
			this->move = Move(move);
			this->hisCount = hisCount;
		}
	};
	//キュー本体
	std::queue<qval> q;
	//指し手の履歴を取得
	auto history = tree.getHistory();
	//trueの間のみキューにノードを追加する
	bool add = true;
	//深さの上限を設定
	int borderHis = history.size() + option.getI("joseki_by_kyokumen_borderHisCount");

	//historyに何もなかったら初期局面を格納しておく
	if (history.size() == 0) {
		tree.set({ "position","startpos" });
		history = tree.getHistory();
	}

	//キューにhistoryの末端を保存
	q.push(qval(history.back(), iojoseki.getKyokumenID(history), history.size(), history.back()->move.binary()));

	while (!q.empty()) {
		//キューの先頭を取り出し
		auto front = q.front();
		q.pop();

		//ノードを定跡から読み込み
		iojoseki.getNode(front.ID, front.node, front.move);
		//深さ上限ならキューへの追加終了
		if (borderHis <= front.hisCount) {
			add = false;
		}

		//子ノードがありそうなら定跡から取得
		if (add && (front.node->getState() == SearchNode::State::E)) {
			const int nextHisCount = front.hisCount + 1;
			if (front.node->children.size() == 0) {
				auto childrenID = iojoseki.getChildren(front.ID);
				SearchNode* list = new SearchNode[childrenID.size()];
				for (int i = 0; i < childrenID.size(); ++i) {
					q.push(qval(&(list[i]), childrenID[i].childKyokmenNumber, nextHisCount, childrenID[i].moveU));
				}
				front.node->children.setChildren(list, childrenID.size());
			}
		}
		else {
			if (front.node->isLeaf()) {
				front.node->setState(SearchNode::State::N);
			}
		}

		nodeCount++;
		if (nodeCount % 1000000 == 0) {
			std::cout << "qsize : " << q.size() << "\tnodeCount : " << nodeCount << std::endl;
		}

	}
	iojoseki.fin();
}

void JosekiByKyokumen::coutFronID(size_t id) {
	iojoseki.init(FILENAME("queue_node.bin"), FILENAME("queue_id.bin"), FILENAME("queue_childposition.bin"), FILENAME("queue_move.bin"));
	iojoseki.printKyokumen(id);
}
