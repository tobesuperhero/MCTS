
#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <math.h>
#include <utility>
#include <map>
#include "jsoncpp/json.h"
using namespace std;

typedef int _GridType;
typedef int _Direction;
typedef int _Color;
typedef pair<int, int> _Coordinate; //Coordinate代表一个坐标点，pair是一个包含俩个int数据的结构体数据成员有first and second
const int SideLength = 8;
const _Color White = -1, Black = 1;
const _GridType WhiteStone = -1, Empty = 0, BlackStone = 1;
const _Direction North = 0, NorthEast = 1, East = 2, SouthEast = 3, South = 4, SouthWest = 5, West = 6, NorthWest = 7;
const int dx[] = { 0, 1, 1, 1, 0, -1, -1, -1 }, dy[] = { -1, -1, 0, 1, 1, 1, 0, -1 };
const clock_t t1 = clock();
const clock_t TIMELIMIT = 0.9*CLOCKS_PER_SEC;
const double UCBC = 0.5;
const int Threshold = 60;

const int RankofPos[SideLength][SideLength] =
{
	200, 3, 20, 15, 15, 20, 3, 200,
	 3, 1, 10, 11, 11, 10, 1, 3,
	20, 10, 12, 12, 12, 12, 10, 20,
	15, 11, 12, 10, 10, 12, 11, 15,
	15, 11, 12, 10, 10, 12, 11, 15,
	20, 10, 12, 12, 12, 12, 10, 20,
	 3, 1, 10, 11, 11, 10, 1, 3,
	200, 3, 20, 15, 15, 20, 3, 200
};
/*
{
	200, 1, 20, 15, 15, 20, 1, 200,
	 1, 3, 10, 11, 11, 10, 3, 1,
	20, 10, 12, 12, 12, 12, 10, 20,
	15, 11, 12, 10, 10, 12, 11, 15,
	15, 11, 12, 10, 10, 12, 11, 15,
	20, 10, 12, 12, 12, 12, 10, 20,
	 1, 3, 10, 11, 11, 10, 3, 1,
	200, 1, 20, 15, 15, 20, 1, 200
};

*/

class State
{
	public:
		State()
		{                    //一个初始化的工作
			for (int i = 0; i<SideLength; i++)
			{
				memset(board[i], Empty, sizeof(board[i]));//初始化？？？？？？
			}
			ColorToPlay = Black;  //初始化当前使用的是黑色的棋子
			StoneNum[0] = StoneNum[1] = 0;
			pre_node = NULL;
			visitcount = totalrank = 0;
		}
		int  board[SideLength][SideLength];// int board[8][8]
		int StoneNum[2];
		int ColorToPlay;
		State* pre_node;
		map<_Coordinate, State*> next_node; //map 是一个索引，next_node->first是一个坐标
		int visitcount, totalrank;
		double value() const
		{
			if (visitcount == 0) return -100;
			return (double)totalrank / visitcount;
		}
};

class Othello
{
public:
	Othello()
	{
		for (int i = 0; i < SideLength; i++)
		{
			memset(board[i], Empty, sizeof(board[i]));
		}
		board[SideLength / 2 - 1][SideLength / 2 - 1] = board[SideLength / 2][SideLength / 2] = WhiteStone;//|w|b|
		board[SideLength / 2][SideLength / 2 - 1] = board[SideLength / 2 - 1][SideLength / 2] = BlackStone;//|b|w|
		StoneNum[0] = StoneNum[1] = 2;
		string str;
#ifndef _BOTZONE_ONLINE
		ifstream is("input.txt");
		getline(is, str);
		is.close();
#else
		getline(cin, str);
#endif
		Json::Reader reader;
		Json::Value input;
		reader.parse(str, input);

		int x, y;
		MyColor = input["requests"][(Json::Value::UInt) 0]["x"].asInt() < 0 ? Black : White;
		ColorToPlay = MyColor;
		int turnID = input["responses"].size();
		for (int i = 0; i < turnID; i++)
		{
			x = input["requests"][i]["x"].asInt();
			y = input["requests"][i]["y"].asInt();
			if (x >= 0)
				PutStone(make_pair(x, y), -MyColor);
			x = input["responses"][i]["x"].asInt();
			y = input["responses"][i]["y"].asInt();
			if (x >= 0)
				PutStone(make_pair(x, y), MyColor);
		}
		x = input["requests"][turnID]["x"].asInt();
		y = input["requests"][turnID]["y"].asInt();
		if (x >= 0)
			PutStone(make_pair(x, y), -MyColor);

		srand((unsigned)time(NULL) + rand());
	}



	int board[SideLength][SideLength];// int board[8][8]
	int StoneNum[2];//White and Black
	int ColorToPlay, MyColor;


	/********************************************备份与恢复*****************************************************************************************/
	void BackUp(State& cache)    //把当前的棋盘状态备份到传过来的参数类型State 的 变量cache
	{
		for (int i = 0; i < SideLength; i++)
		{
			memcpy(cache.board[i], board[i], sizeof(board[i]));
		}
		cache.ColorToPlay = ColorToPlay;
		cache.StoneNum[0] = StoneNum[0];
		cache.StoneNum[1] = StoneNum[1];
	}
	void Recovery(const State& cache)  //利用参数cache恢复，与BackUp函数相比是一个相反的函数
	{
		for (int i = 0; i < SideLength; i++)
		{
			memcpy(board[i], cache.board[i], sizeof(board[i]));
		}
		ColorToPlay = cache.ColorToPlay;
		StoneNum[0] = cache.StoneNum[0];
		StoneNum[1] = cache.StoneNum[1];
	}
	/*************************************************************************************************************************************/

//调用时没有说明方向
	bool Available(const _Coordinate& pos, int color)
	{
		if (board[pos.second][pos.first] != Empty) return false;
		for (int dir = North; dir <= NorthWest; dir++)
		{
			if (Available(pos, dir, color)) return true; //在没有说明方向的情况下，8个方向都会检查
		}
		return false;
	}
	//调用时说明了方向
	bool Available(const _Coordinate& pos, int dir, int color)
	{
		if (board[pos.second][pos.first] != Empty) return false;
		pair<_Coordinate, bool> res = NextPos(pos, dir);
		if (!res.second) return false;
		_Coordinate np = res.first;
		int temp = (color == White ? WhiteStone : BlackStone);
		if (board[np.second][np.first] == temp || board[np.second][np.first] == Empty) return false;

		//该方向相邻子为同色或空
		do
		{
			res = NextPos(res.first, dir);
			if (!res.second) return false;
			np = res.first;
			if (board[np.second][np.first] == temp) return true;
			if (board[np.second][np.first] == Empty) return false;
		} while (res.second);
		return false;
	}




	//isEnd()函数用来判别当前棋盘是否已经结束
	bool isEnd()
	{
		if (StoneNum[0] + StoneNum[1] >= SideLength*SideLength) return true;
		for (int i = 0; i < SideLength; i++)
		{
			for (int j = 0; j < SideLength; j++)
			{
				if (Available(make_pair(i, j), Black) || Available(make_pair(i, j), White)) return false;
			}
		}
		return true;
	}

	//判断谁赢了
	int EndJudge(_Color color)
	{
		if (StoneNum[(color + 1) / 2] > StoneNum[(1 - color) / 2]) return 1;
		if (StoneNum[(color + 1) / 2] == StoneNum[(1 - color) / 2]) return 0;
		if (StoneNum[(color + 1) / 2] < StoneNum[(1 - color) / 2]) return -1;
	}


	// Upper Confidence Bounds 作为一个公式所计算出来的值来权衡

	double UCB(const State* current_node, const State* next_node)
	{
		if (next_node->visitcount == 0) return -100;
		//UCB公式
		return next_node->value() + UCBC*sqrt(log(current_node->visitcount) / next_node->visitcount);
	}



	pair<_Coordinate, bool> NextPos(const _Coordinate& pos, _Direction dir)
	{
		if (dir < 0 || dir>7) return make_pair(pos, false);
		_Coordinate np = pos;
		np.first += dx[dir];  //8个方向，每一个方向都会有自己方向的x或者y的加减值
		np.second += dy[dir];
		if (np.first < 0 || np.first >= SideLength || np.second < 0 || np.second >= SideLength) return make_pair(pos, false);// 越界了，返回失败！！
		return make_pair(np, true);
	}

	//落子
	bool PutStone(_Coordinate pos, int color)
	{
		if (pos == make_pair(-1, -1)) return true;
		_GridType temp = (color == White ? WhiteStone : BlackStone); //存储当前的color，这难道不应该都是黑色的吗（1）？？？？？？
		pair<_Coordinate, bool> res;
		_Coordinate np;
		bool flag = false;
		for (_Direction dir = North; dir <= NorthWest; dir++)
		{
			if (Available(pos, dir, color))
			{
				res = NextPos(pos, dir);
				np = res.first;  //得到一个坐标，检测这个坐标是否合理

				while (board[np.second][np.first] != temp)
				{
					board[np.second][np.first] = temp;
					res = NextPos(np, dir);  //NextPos函数里面确保了棋子没有越界
					np = res.first;  //dir方向上的棋子
					StoneNum[(color + 1) / 2]++; //如果color是黑的（1）话 StoneNum[1]++，白（-1）的话，就是StoneNum[0]++
					StoneNum[(1 - color) / 2]--;   //如果color是黑的（1）话 StoneNum[0]++，白（-1）的话，就是StoneNum[1]++
				}
				flag = true; //能跳出上面那个while循环，说明一直找到了这个方向上为空（0）的待落子点
			}
		}
			if (flag)   //正确跳出了上面的那个循环，准备模拟落子
			{
				board[pos.second][pos.first] = temp;
				StoneNum[(color + 1) / 2]++;
				return true;
			}
			else
				return false;
	}

	pair<State*, bool> Selection(State* current_node)
	{
		if (isEnd())
			return make_pair(current_node, false);
		if (current_node->visitcount < Threshold) 
			return make_pair(current_node, false);
		State* next = current_node;
		double temp, mUCB = -200;
		for (map<_Coordinate, State*>::iterator iter = current_node->next_node.begin(); iter != current_node->next_node.end(); iter++)
		{
			temp = UCB(current_node, iter->second);
			if (temp > mUCB)
			{
				next = iter->second;
				mUCB = temp;
			}
		}
		Recovery(*next);
		if (next == current_node) 
			return make_pair(next, false);
		else
			return make_pair(next, true);
	}
	State* Expand(State* current_node)
	{
		if (isEnd()) 
			return current_node;
		map<_Coordinate, int> alterlist;
		_Coordinate choice;
		int temp, sum = 0;
		for (int i = 0; i < SideLength; i++)
		{
			for (int j = 0; j < SideLength; j++)
			{
				if (Available(make_pair(i, j), ColorToPlay))
				{
					alterlist.insert(make_pair(make_pair(i, j), RankofPos[i][j]));
					sum += RankofPos[i][j];
				}
			}
		}
		if (sum == 0)
		{
			choice = make_pair(-1, -1);
		}
		else
		{
			temp = rand() % sum;
			for (map<_Coordinate, int>::iterator iter = alterlist.begin(); iter != alterlist.end(); iter++)
			{
				temp -= iter->second;
				if (temp < 0)
				{
					choice = iter->first;
					break;
				}
			}
		}
		map<_Coordinate, State*>::iterator it = current_node->next_node.find(choice);
		if (it != current_node->next_node.end())
		{
			Recovery(*(it->second));
			return it->second;
		}
		State *next = new State();
		PutStone(choice, ColorToPlay);
		ColorToPlay = -ColorToPlay;
		next->pre_node = current_node;
		BackUp(*next);
		current_node->next_node.insert(make_pair(choice, next));
		return next;
	}
	void Simulation(const State* current_node, int rank[2])//rollout policy
	{
		map<_Coordinate, int> alterlist;
		_Coordinate choice;
		int temp, sum = 0;
		while (!isEnd())
		{
			alterlist.clear();
			sum = 0;
			for (int i = 0; i < SideLength; i++)
			{
				for (int j = 0; j < SideLength; j++)
				{
					if (Available(make_pair(i, j), ColorToPlay))
					{
						alterlist.insert(make_pair(make_pair(i, j), RankofPos[i][j]));
						sum += RankofPos[i][j];
					}
				}
			}
			if (sum == 0)
			{
				choice = make_pair(-1, -1);
			}
			else
			{
				temp = rand() % sum;
				for (map<_Coordinate, int>::iterator iter = alterlist.begin(); iter != alterlist.end(); iter++)
				{
					temp -= iter->second;
					if (temp < 0)
					{
						choice = iter->first;
						break;
					}
				}
			}
			PutStone(choice, ColorToPlay);
			ColorToPlay = -ColorToPlay;
		}
		rank[0] = EndJudge(White);
		rank[1] = EndJudge(Black);
		Recovery(*current_node);
		return;
	}
	void BackPropagation(State* current_node, const int r[2])
	{
		while (current_node != NULL)
		{
			current_node->visitcount++;
			current_node->totalrank += r[(1 - current_node->ColorToPlay) / 2];
			current_node = current_node->pre_node;
		}
		return;
	}

	


	

	/******************************MCTS******************************************/
	_Coordinate MCTS()
	{
		State *root_node, *current_node;
		root_node = new State();
		pair<State*, bool> res;
		int r[2];
		BackUp(*root_node);
		while (clock() - t1 < TIMELIMIT)  //TIMELIMIT时间内做出落子决定
		{
			current_node = root_node;  //把当前结点作为根节点开始形成MCT
			do
			{
				res = Selection(current_node);
				current_node = res.first;
			} while (res.second);//为真时
			current_node = Expand(current_node);
			Simulation(current_node, r);
			BackPropagation(current_node, r);
			Recovery(*root_node);
		}
		_Coordinate choice = make_pair(-1, -1);
		double temp, best = -100;
#ifndef _BOTZONE_ONLINE
		cout << root_node->visitcount << endl;
#endif
		for (map<_Coordinate, State*>::iterator iter = root_node->next_node.begin(); iter != root_node->next_node.end(); iter++)
		{
			temp = iter->second->value();
#ifndef _BOTZONE_ONLINE
			cout << iter->first.second << "," << iter->first.first << " " << iter->second->visitcount << " " << temp << endl;
#endif
			if (temp > best)
			{
				choice = iter->first;
				best = temp;
			}
		}
		return choice;
	}
};

	/**********************************MCTS**************************************/

	/***************************************************************************/

	/***************************************************************************/
	int main()
	{
		_Coordinate choice;
		Othello othello;
		choice = othello.MCTS();
		//choice = othello.MakeChoice();
		/************************************************************************/
		Json::Value ret;
		ret["response"]["x"] = choice.first;
		ret["response"]["y"] = choice.second;
		Json::FastWriter writer;
		cout << writer.write(ret) << endl;
		return 0;
	}

