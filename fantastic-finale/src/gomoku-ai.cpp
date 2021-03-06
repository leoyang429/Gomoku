#include "gomoku-ai.hpp"
using std::vector;
using std::priority_queue;
  
GomokuAI::GomokuAI() {
    for (int i = 0; i < kBoardSize; ++i) {
        for (int j = 0; j < kBoardSize; ++j) {
            for (int k = 0; k < 2; ++k) {
                zobrist_table[i][j][k] = RandomUnsignedLL();
            }
        }
    }
    
    is_first_player = false;
    is_empty_board = is_first_player;
}

ll GomokuAI::RandomUnsignedLL()
{
    std::uniform_int_distribution<ll> dist(0, UINT64_MAX);
    return dist(mt);
}

void GomokuAI::SetIsFirstPlayer(bool is_first_player) {
    is_empty_board = this->is_first_player = is_first_player;
}

bool GomokuAI::GetIsFirstPlayer() {
    return is_first_player;
}

int GomokuAI::GetScore(const vector<vector<int> >& board, int player, bool is_enemy_next) {
    
    int score = 10;
    int consecutives[kBoardSize][kBoardSize] = {0};
    
    for (int i = 0; i < kBoardSize; ++i) {
        for (int j = 0; j < kBoardSize; ++j) {
            if (board[i][j] == player) {
                for (int k = 0; k < kDirections; ++k) {
                    
                    int previous_x = i - kXChange[k], previous_y = j - kYChange[k];
                    if (previous_x >= 0 && previous_x < kBoardSize && previous_y >= 0 && previous_y < kBoardSize
                        && board[previous_x][previous_y] == player) {
                        continue;
                    }
                    
                    int consecutive = 1;
                    int x = i, y = j;
                    bool skipped_one_intersect = false;
                    int first_half_consecutive = 0;
                    
                    while(consecutive <= kBoardSize) {
                        x += kXChange[k];
                        y += kYChange[k];
                        if (x < 0 || y < 0 || x >= kBoardSize || y >= kBoardSize) {
                            break;
                        }
                        if (board[x][y] != player && (board[x][y] != kNoPlayer || skipped_one_intersect)) {
                            break;
                        }
                        if (board[x][y] == kNoPlayer) {
                            first_half_consecutive = consecutive;
                            skipped_one_intersect = true;
                        } else {
                            ++ consecutive;
                        }
                    }
                    
                    if (board[x - kXChange[k]][y - kYChange[k]] == kNoPlayer) {
                        x -= kXChange[k];
                        y -= kYChange[k];
                        skipped_one_intersect = false;
                    }
                    
                    if (consecutive >= 5 && skipped_one_intersect) {
                        consecutive = first_half_consecutive;
                    }
                    
                    if (consecutive >= 5) {
                        return kWinningScore;
                    }
                    
                    if (x < 0 || y < 0 || x >= kBoardSize || y >= kBoardSize ||
                        board[x][y] != kNoPlayer) {
                        -- consecutive;
                    }
                    if (previous_x < 0 || previous_y < 0 || previous_x >= kBoardSize || previous_y >= kBoardSize ||
                        board[previous_x][previous_y] != kNoPlayer) {
                        -- consecutive;
                    }
                    
                    if (consecutive >= 4 && skipped_one_intersect) {
                        consecutive = first_half_consecutive;
                    }
                    
                    if (consecutive >= 0) {
                        if (is_enemy_next) {
                            score += kConsecutiveScore[consecutive];
                        } else {
                            score += kEnemyConsecutiveScore[consecutive];
                        }
                    }
                    
                }
            }
        }
    }
    
    return score;
}

ll GomokuAI::GetHash(const vector<vector<int> >& board) {
    // it isn't called yet
    // but it's a cool way to hash a chess board
    
    hash = 0;
    for (int i = 0; i < kBoardSize; ++i) {
        for (int j = 0; j < kBoardSize; ++j) {
            if (board[i][j] != kNoPlayer) {
                hash ^= zobrist_table[i][j][board[i][j] - 1];
            }
        }
    }
    return hash;
}

int GomokuAI::Move(vector<vector<int> >& board) {
    
    // always start from the center
    if (is_empty_board) {
        is_empty_board = false;
        return kBoardSize * kBoardSize / 2;
    }
    
    int hash_code = GetHash(board);
    
    vector<int> possible_moves = GetPossibleMoves(board);
    
    int best_move = -1;
    int highest_score = -kWinningScore;
    
    for (int move: possible_moves) {
        int x = move % kBoardSize, y = move / kBoardSize;
        board[x][y] = 2 - is_first_player;
        int score = Minimax(board, 2, highest_score);
        if (score > highest_score) {
            highest_score = score;
            best_move = move;
        }
        board[x][y] = kNoPlayer;
    }
    
    return model[hash_code] = best_move;
}

// refer to https://en.wikipedia.org/wiki/Minimax
int GomokuAI::Minimax(vector<vector<int> >& board, int depth, int alpha_beta) {
    
    if (HasWon(board, (depth & 1) ? 2 - !is_first_player : 2 - is_first_player)) {
        return 2 * pow(-1, (depth & 1)) * kWinningScore;
    }
    
    if (depth == kMaxSearchDepth) {
        return GetScore(board, 2 - is_first_player, true) - GetScore(board, 2 - !is_first_player, false);
    }
    
    if (depth & 1) {
        
        vector<int> potential_moves = GetPossibleMoves(board);
        vector<int> possible_moves;
        
        priority_queue<pair<int, int> > pq;
        for (int move: potential_moves) {
            int x = move % kBoardSize, y = move / kBoardSize;
            board[x][y]=2-is_first_player;
            pq.push(make_pair(GetScore(board, 2 - is_first_player, true)
                              - GetScore(board, 2 - !is_first_player, false), move));
            board[x][y]=kNoPlayer;
        }
        while(!pq.empty()){
            possible_moves.push_back(pq.top().second);
            pq.pop();
        }
        
        int highest_score = -kWinningScore;
        
        for (int move: possible_moves) {
            int x = move % kBoardSize, y = move / kBoardSize;
            board[x][y] = 2 - is_first_player;
            int score = Minimax(board, depth + 1, highest_score);
            if (score > highest_score) {
                highest_score = score;
            }
            board[x][y] = kNoPlayer;
            
            if (highest_score > alpha_beta) {
                return highest_score;
            }
        }
        
        return highest_score;
        
    } else {
        
        vector<int> potential_moves = GetPossibleMoves(board);
        vector<int> possible_moves;
        
        priority_queue<pair<int, int> > pq;
        for (int move: potential_moves) {
            int x = move % kBoardSize, y = move / kBoardSize;
            board[x][y]=2-!is_first_player;
            pq.push(make_pair(GetScore(board, 2 - !is_first_player, true)
                              - GetScore(board, 2 - is_first_player, false), move));
            board[x][y]=kNoPlayer;
        }
        while(!pq.empty()){
            possible_moves.push_back(pq.top().second);
            pq.pop();
        }
        
        int lowest_score = kWinningScore;
        
        for (int move: possible_moves) {
            int x = move % kBoardSize, y = move / kBoardSize;
            board[x][y] = 2 - !is_first_player;
            int score = Minimax(board, depth + 1, lowest_score);
            if (score < lowest_score) {
                lowest_score = score;
            }
            board[x][y] = kNoPlayer;
            
            if (lowest_score < alpha_beta) {
                return lowest_score;
            }
        }
        
        return lowest_score;
    }
    
}

vector<int> GomokuAI::GetPossibleMoves(vector<vector<int> >& board) {
    
    vector<int> possible_moves;
    
    for (int move = 0; move < kBoardSize * kBoardSize; ++ move) {
        int x = move % kBoardSize, y = move / kBoardSize;
        if (board[x][y] == kNoPlayer) {
            bool close_enough = false;
            for (int i = x - 1; i <= x + 1; ++ i) {
                for (int j = y - 1; j <= y + 1; ++ j) {
                    if (i >= 0 && j >= 0 && i < kBoardSize && j < kBoardSize
                        && board[i][j] != kNoPlayer) {
                        close_enough = true;
                        break;
                    }
                }
            }
            
            if (close_enough) {
                possible_moves.push_back(move);
            }
        }
    }
    
    return possible_moves;
}

int GomokuAI::HasWon(const vector<vector<int> >& board, int player) {
    for (int i = 0; i < kBoardSize; ++i) {
        for (int j = 0; j < kBoardSize; ++j) {
            if (board[i][j] == player) {
                for (int k = 0; k < kDirections; ++k) {
                    
                    int previous_x = i - kXChange[k], previous_y = j - kYChange[k];
                    if (previous_x >= 0 && previous_x < kBoardSize && previous_y >= 0 && previous_y < kBoardSize
                        && board[previous_x][previous_y] == player) {
                        continue;
                    }
                    
                    int consecutive = 1;
                    int x = i, y = j;
                    
                    while(consecutive <= kBoardSize) {
                        x += kXChange[k];
                        y += kYChange[k];
                        if (x < 0 || y < 0 || x >= kBoardSize || y >= kBoardSize) {
                            break;
                        }
                        if (board[x][y] != player) {
                            break;
                        }
                        ++ consecutive;
                    }
                    
                    if (consecutive >= 5) {
                        return true;
                    }
                }
            }
        }
    }
    
    return false;
}
