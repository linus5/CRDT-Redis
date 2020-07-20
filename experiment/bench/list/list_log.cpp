//
// Created by admin on 2020/1/13.
//
#include "list_log.h"

double list_log::diff(const list_log::element &e, const redisReply *r)
{
    if (e.content != r->element[1]->str) return 1;
    if (e.font != atoi(r->element[2]->str)) return 0.5;    // NOLINT
    if (e.size != atoi(r->element[3]->str)) return 0.5;    // NOLINT
    if (e.color != atoi(r->element[4]->str)) return 0.5;   // NOLINT
    int p = atoi(r->element[5]->str);                      // NOLINT
    if (e.bold != (bool)(p & BOLD)) return 0.5;            // NOLINT
    if (e.italic != (bool)(p & ITALIC)) return 0.5;        // NOLINT
    if (e.underline != (bool)(p & UNDERLINE)) return 0.5;  // NOLINT
    return 0;
}

void list_log::read_list(redisReply_ptr &r)
{
    list<element *> doc_read;
    {
        lock_guard<mutex> lk(mtx);
        for (auto &e_p : document)
            doc_read.emplace_back(e_p.get());
    }

    int len = doc_read.size();
    int r_len = r->elements;
    // Levenshtein distance
    vector<vector<double> > dp(len + 1, vector<double>(r_len + 1, 0));
    for (int i = 1; i <= len; i++)
        dp[i][0] = i;
    for (int j = 1; j <= r_len; j++)
        dp[0][j] = j;
    auto iter = doc_read.begin();
    for (int i = 1; i <= len; i++)
    {
        for (int j = 1; j <= r_len; j++)
            dp[i][j] = min(dp[i - 1][j - 1] + diff(**iter, r->element[j - 1]),
                           min(dp[i][j - 1] + 1, dp[i - 1][j] + 1));
        iter++;
    }
    double distance = dp[len][r_len];

    {
        lock_guard<mutex> lk(dis_mtx);
        distance_log.emplace_back(len, distance);
    }
}

void list_log::overhead(int o)
{
    int num;
    {
        lock_guard<mutex> lk(mtx);
        num = document.size();
    }
    lock_guard<mutex> lk(ovhd_mtx);
    overhead_log.emplace_back(num, o);
}

void list_log::write_file()
{
    ostringstream stream;
    stream << dir << "/ovhd.csv";
    ofstream ovhd(stream.str(), ios::out | ios::trunc);
    for (auto &o : overhead_log)
        ovhd << get<0>(o) << "," << get<1>(o) << "\n";

    stream.str("");
    stream << dir << "/distance.csv";
    ofstream distance(stream.str(), ios::out | ios::trunc);
    for (auto &o : distance_log)
        distance << get<0>(o) << "," << get<1>(o) << "\n";
}

void list_log::insert(string &prev, string &name, string &content, int font, int size, int color,
                      bool bold, bool italic, bool underline)
{
    lock_guard<mutex> lk(mtx);
    if (ele_map.find(prev) == ele_map.end() || ele_map.find(name) != ele_map.end()) return;
    auto it_next = prev.empty() ? document.begin() : ele_map[prev];
    if (it_next != document.begin()) it_next++;
    document.emplace(it_next, new element(content, font, size, color, bold, italic, underline));
    it_next--;
    ele_map[name] = it_next;
}

void list_log::update(string &name, string &upd_type, int value)
{
    lock_guard<mutex> lk(mtx);
    if (ele_map.find(name) != ele_map.end())
    {
        auto &e = *ele_map[name];
        if (upd_type == "font")
            e->font = value;
        else if (upd_type == "size")
            e->size = value;
        else if (upd_type == "color")
            e->color = value;
        else if (upd_type == "bold")
            e->bold = value;
        else if (upd_type == "italic")
            e->italic = value;
        else if (upd_type == "underline")
            e->underline = value;
    }
}

void list_log::remove(string &name)
{
    lock_guard<mutex> lk(mtx);
    if (ele_map.find(name) != ele_map.end())
    {
        auto &e = ele_map[name];
        document.erase(e);
        ele_map.erase(name);
    }
}

string list_log::random_get()
{
    lock_guard<mutex> lk(mtx);
    if (ele_map.empty()) return string("null");
    int pos = intRand(ele_map.size() + 1);  // NOLINT
    if (pos == ele_map.size()) return string("null");
    auto random_it = next(begin(ele_map), pos);
    return random_it->first;
}
