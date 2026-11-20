
#include <wx/wx.h>
#include <wx/notebook.h>
#include <wx/wx.h>
#include <wx/numdlg.h>
#include <wx/dcbuffer.h>
#include <cmath>
#include <map>
#include <set>
#include <limits>
#include <fstream>
#include <functional>
#include <sstream>
#include <string>
#include <vector>
#include <cctype>

#include "graph.hpp"
#include "../AVL_Tree/avl_tree.hpp"

using int_graph = Graph<int, int, int, AVL_Tree>;

enum class Mode { AddVertex, AddEdge, Move, Remove, Path };

static constexpr int R = 20;  // радиус вершины

double DistToSegment(wxPoint p, wxPoint a, wxPoint b) {
    double dx = b.x - a.x, dy = b.y - a.y;
    double len2 = dx * dx + dy * dy;
    double t = len2 == 0 ? 0 : ((p.x - a.x) * dx + (p.y - a.y) * dy) / len2;
    t = std::max(0.0, std::min(1.0, t));
    double px = a.x + t * dx, py = a.y + t * dy;
    return std::hypot(p.x - px, p.y - py);
}

class GraphCanvas : public wxPanel {
public:
    explicit GraphCanvas(wxWindow* parent) : wxPanel(parent) {
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        Bind(wxEVT_PAINT, &GraphCanvas::OnPaint, this);
        Bind(wxEVT_LEFT_DOWN, &GraphCanvas::OnDown, this);
        Bind(wxEVT_LEFT_UP, &GraphCanvas::OnUp, this);
        Bind(wxEVT_MOTION, &GraphCanvas::OnMove, this);
    }

    void SetMode(Mode m) {
        m_mode = m;
        m_from = -1;
        ClearPath();
        Refresh();
    }

private:
    int_graph m_graph;
    std::map<int, wxPoint> m_pos;
    int m_nextId = 1;
    Mode m_mode = Mode::AddVertex;

    int m_from = -1;     
    bool m_drag = false;

    std::map<int, int> m_dist, m_prev;
    std::set<std::pair<int, int>> m_pathEdges;

    void ClearPath() {
        m_dist.clear();
        m_prev.clear();
        m_pathEdges.clear();
    }

    int HitVertex(wxPoint p) const {
        for (const auto& [id, pos] : m_pos)
            if (std::hypot(p.x - pos.x, p.y - pos.y) <= R) return id;
        return -1;
    }

    // смещение для пары рёбер, чтобы не сливались
    void EdgeEnds(int a, int b, wxPoint& s, wxPoint& e) const {
        wxPoint pa = m_pos.at(a), pb = m_pos.at(b);
        double dx = pb.x - pa.x, dy = pb.y - pa.y;
        double len = std::hypot(dx, dy);
        if (len < 1) { s = pa; e = pb; return; }
        double ux = dx / len, uy = dy / len;
        double nx = -uy * 6, ny = ux * 6;
        s = wxPoint(pa.x + ux * R + nx, pa.y + uy * R + ny);
        e = wxPoint(pb.x - ux * R + nx, pb.y - uy * R + ny);
    }

    bool HitEdge(wxPoint p, int& a, int& b) const {
        auto edges = m_graph.get_edges();
        for (const auto& k : edges) {
            wxPoint s, e;
            EdgeEnds(k.GetFirst(), k.GetSecond(), s, e);
            if (DistToSegment(p, s, e) < 6) {
                a = k.GetFirst();
                b = k.GetSecond();
                return true;
            }
        }
        return false;
    }

    void RunDijkstra(int start) {
        ClearPath();
        ArraySequence<int> prev;
        ArraySequence<int> dist = m_graph.dijkstra(start, prev);
        auto vertices = m_graph.get_vertices();   // тот же порядок, что внутри dijkstra
        for (unsigned i = 0; i < vertices.GetLength(); ++i) {
            m_dist[vertices[i]] = dist[i];
            m_prev[vertices[i]] = prev[i];
        }
    }

    void ShowPathTo(int start, int target) {
        m_pathEdges.clear();
        if (m_dist[target] == std::numeric_limits<int>::max()) {
            wxMessageBox(wxString::FromUTF8("Путь недостижим"), wxString::FromUTF8("Дейкстра"));
            return;
        }
        int cur = target;
        while (cur != start) {
            int p = m_prev[cur];
            m_pathEdges.insert({p, cur});
            cur = p;
        }
    }

    void OnDown(wxMouseEvent& ev) {
        wxPoint p = ev.GetPosition();
        int hit = HitVertex(p);
        try {
            switch (m_mode) {
            case Mode::AddVertex:
                if (hit == -1) {
                    m_graph.add_vertex(m_nextId, 0);
                    m_pos[m_nextId++] = p;
                }
                break;

            case Mode::AddEdge:
                if (hit == -1) { m_from = -1; break; }
                if (m_from == -1) { m_from = hit; break; }
                if (m_from != hit) {
                    long w = wxGetNumberFromUser(wxString::FromUTF8("Вес ребра"),
                                                 wxString::FromUTF8("Вес:"),
                                                 wxString::FromUTF8("Новое ребро"),
                                                 1, 0, 1000000, this);
                    if (w >= 0) {
                        if (m_graph.contains_edge(m_from, hit))
                            m_graph.rewriting_weihgt(m_from, hit, (int)w);  // ребро есть — меняем вес
                        else
                            m_graph.add_edge(m_from, hit, (int)w);
                    }
                }
                m_from = -1;
                break;

            case Mode::Move:
                if (hit != -1) { m_from = hit; m_drag = true; }
                break;

            case Mode::Remove: {
                int a, b;
                if (hit != -1) {
                    m_graph.delete_vertex(hit);
                    m_pos.erase(hit);
                } else if (HitEdge(p, a, b)) {
                    m_graph.delete_edge(a, b);
                }
                break;
            }

            case Mode::Path:
                if (hit == -1) break;
                if (m_from == -1) {            // первый клик — старт
                    m_from = hit;
                    RunDijkstra(hit);
                } else {                       // следующие клики — цель
                    ShowPathTo(m_from, hit);
                }
                break;
            }
        } catch (const std::exception& e) {
            wxMessageBox(wxString::FromUTF8(e.what()), wxString::FromUTF8("Ошибка"), wxICON_ERROR);
        }
        Refresh();
    }

    void OnUp(wxMouseEvent&) { if (m_mode == Mode::Move) { m_drag = false; m_from = -1; } }

    void OnMove(wxMouseEvent& ev) {
        if (m_mode == Mode::Move && m_drag && m_from != -1 && ev.Dragging()) {
            m_pos[m_from] = ev.GetPosition();
            Refresh();
        }
    }

    void DrawArrow(wxDC& dc, wxPoint s, wxPoint e) {
        dc.DrawLine(s, e);
        double ang = std::atan2(e.y - s.y, e.x - s.x);
        const double L = 10, A = 0.4;
        wxPoint a1(e.x - L * std::cos(ang - A), e.y - L * std::sin(ang - A));
        wxPoint a2(e.x - L * std::cos(ang + A), e.y - L * std::sin(ang + A));
        dc.DrawLine(e, a1);
        dc.DrawLine(e, a2);
    }

    void OnPaint(wxPaintEvent&) {
        wxAutoBufferedPaintDC dc(this);
        dc.SetBackground(*wxWHITE_BRUSH);
        dc.Clear();

        // рёбра
        auto edges = m_graph.get_edges();
        for (const auto& k : edges) {
            int a = k.GetFirst(), b = k.GetSecond();
            bool onPath = m_pathEdges.count({a, b}) > 0;
            dc.SetPen(wxPen(onPath ? *wxRED : *wxBLACK, onPath ? 3 : 1));
            wxPoint s, e;
            EdgeEnds(a, b, s, e);
            DrawArrow(dc, s, e);
            dc.SetTextForeground(onPath ? *wxRED : wxColour(0, 0, 160));
            dc.DrawText(wxString::Format("%d", m_graph.get_weight_edge(a, b)),
                        (s.x + e.x) / 2 + 3, (s.y + e.y) / 2 - 16);
        }

        // вершины
        for (const auto& [id, pos] : m_pos) {
            bool sel = (id == m_from);
            dc.SetPen(*wxBLACK_PEN);
            dc.SetBrush(sel ? wxBrush(wxColour(255, 220, 120)) : wxBrush(wxColour(200, 225, 255)));
            dc.DrawCircle(pos, R);
            dc.SetTextForeground(*wxBLACK);
            wxString t = wxString::Format("%d", id);
            wxSize ts = dc.GetTextExtent(t);
            dc.DrawText(t, pos.x - ts.x / 2, pos.y - ts.y / 2);

            auto it = m_dist.find(id);
            if (it != m_dist.end()) {
                wxString d = it->second == std::numeric_limits<int>::max()
                                 ? wxString::FromUTF8("∞")
                                 : wxString::Format("%d", it->second);
                dc.SetTextForeground(wxColour(0, 120, 0));
                dc.DrawText("d=" + d, pos.x - R, pos.y + R + 2);
            }
        }
    }
};


using str_graph = Graph<int, std::string, std::string, AVL_Tree>;

static wxString U8(const char* s) { return wxString::FromUTF8(s); }

static std::string LabelFor(int n) {
    std::string s;
    do {
        s.insert(s.begin(), char('A' + n % 26));
        n = n / 26 - 1;
    } while (n >= 0);
    return s;
}

class StringGraphCanvas : public wxPanel {
public:
    std::function<void()> onPathChanged;

    explicit StringGraphCanvas(wxWindow* parent) : wxPanel(parent) {
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        Bind(wxEVT_PAINT, &StringGraphCanvas::OnPaint, this);
        Bind(wxEVT_LEFT_DOWN, &StringGraphCanvas::OnDown, this);
        Bind(wxEVT_LEFT_UP, &StringGraphCanvas::OnUp, this);
        Bind(wxEVT_MOTION, &StringGraphCanvas::OnMove, this);
    }

    void SetMode(Mode m) {
        m_mode = m;
        m_from = -1;
        m_drag = false;
        Refresh();
    }

    std::size_t PathLength() const { return m_path.size(); }

    void ClearPath() {
        m_path.clear();
        Notify();
        Refresh();
    }

    wxString PathText() const {
        wxString s;
        for (std::size_t i = 0; i < m_path.size(); ++i) {
            if (i) s += U8(" → ");
            s += U8(m_graph.get_vertex_data(m_path[i]).c_str());
        }
        return s;
    }

    bool SetPathByLabels(const std::vector<std::string>& labels, std::string& bad) {
        std::vector<int> ids;
        for (const auto& label : labels) {
            int found = -1;
            for (const auto& [id, pos] : m_pos) {
                if (m_graph.get_vertex_data(id) == label) { found = id; break; }
            }
            if (found == -1) { bad = label; return false; }
            ids.push_back(found);
        }
        m_path = ids;
        Notify();
        Refresh();
        return true;
    }

    bool BuildString(std::string& out) const {
        if (m_path.size() < 2) return false;
        ArraySequence<int> seq;
        for (int id : m_path) seq.Append(id);
        out = m_graph.get_string_from_path(seq);
        return !out.empty();   
    }

private:
    str_graph m_graph;
    std::map<int, wxPoint> m_pos;
    int m_next = 0;                 
    Mode m_mode = Mode::AddVertex;
    int m_from = -1;
    bool m_drag = false;
    std::vector<int> m_path;

    void Notify() { if (onPathChanged) onPathChanged(); }

    int HitVertex(wxPoint p) const {
        for (const auto& [id, pos] : m_pos)
            if (std::hypot(p.x - pos.x, p.y - pos.y) <= R) return id;
        return -1;
    }

    void EdgeEnds(int a, int b, wxPoint& s, wxPoint& e) const {
        wxPoint pa = m_pos.at(a), pb = m_pos.at(b);
        double dx = pb.x - pa.x, dy = pb.y - pa.y;
        double len = std::hypot(dx, dy);
        if (len < 1) { s = pa; e = pb; return; }
        double ux = dx / len, uy = dy / len;
        double nx = -uy * 6, ny = ux * 6;
        s = wxPoint(pa.x + ux * R + nx, pa.y + uy * R + ny);
        e = wxPoint(pb.x - ux * R + nx, pb.y - uy * R + ny);
    }

    bool HitEdge(wxPoint p, int& a, int& b) const {
        auto edges = m_graph.get_edges();
        for (const auto& k : edges) {
            wxPoint s, e;
            EdgeEnds(k.GetFirst(), k.GetSecond(), s, e);
            if (DistToSegment(p, s, e) < 6) {
                a = k.GetFirst();
                b = k.GetSecond();
                return true;
            }
        }
        return false;
    }

    void OnDown(wxMouseEvent& ev) {
        wxPoint p = ev.GetPosition();
        int hit = HitVertex(p);
        try {
            switch (m_mode) {
            case Mode::AddVertex:
                if (hit == -1) {
                    int id = m_next++;
                    m_graph.add_vertex(id, LabelFor(id)); 
                    m_pos[id] = p;
                }
                break;

            case Mode::AddEdge:
                if (hit == -1) { m_from = -1; break; }
                if (m_from == -1) { m_from = hit; break; }
                if (m_from != hit) {
                    wxString initial;
                    if (m_graph.contains_edge(m_from, hit))
                        initial = U8(m_graph.get_weight_edge(m_from, hit).c_str());
                    wxTextEntryDialog dlg(this, U8("Строка — вес ребра:"), U8("Новое ребро"), initial);
                    if (dlg.ShowModal() == wxID_OK) {
                        std::string w = dlg.GetValue().utf8_string();
                        if (!w.empty()) {
                            if (m_graph.contains_edge(m_from, hit))
                                m_graph.rewriting_weihgt(m_from, hit, w);
                            else
                                m_graph.add_edge(m_from, hit, w);
                        }
                    }
                }
                m_from = -1;
                break;

            case Mode::Move:
                if (hit != -1) { m_from = hit; m_drag = true; }
                break;

            case Mode::Remove: {
                int a, b;
                if (hit != -1) {
                    m_graph.delete_vertex(hit);
                    m_pos.erase(hit);
                    m_path.clear();
                    Notify();
                } else if (HitEdge(p, a, b)) {
                    m_graph.delete_edge(a, b);
                    m_path.clear();
                    Notify();
                }
                break;
            }

            case Mode::Path:          
                if (hit != -1) {
                    m_path.push_back(hit);
                    Notify();
                }
                break;
            }
        } catch (const std::exception& e) {
            wxMessageBox(U8(e.what()), U8("Ошибка"), wxICON_ERROR);
        }
        Refresh();
    }

    void OnUp(wxMouseEvent&) { if (m_mode == Mode::Move) { m_drag = false; m_from = -1; } }

    void OnMove(wxMouseEvent& ev) {
        if (m_mode == Mode::Move && m_drag && m_from != -1 && ev.Dragging()) {
            m_pos[m_from] = ev.GetPosition();
            Refresh();
        }
    }

    void DrawArrow(wxDC& dc, wxPoint s, wxPoint e) {
        dc.DrawLine(s, e);
        double ang = std::atan2(e.y - s.y, e.x - s.x);
        const double L = 10, A = 0.4;
        wxPoint a1(e.x - L * std::cos(ang - A), e.y - L * std::sin(ang - A));
        wxPoint a2(e.x - L * std::cos(ang + A), e.y - L * std::sin(ang + A));
        dc.DrawLine(e, a1);
        dc.DrawLine(e, a2);
    }

    void OnPaint(wxPaintEvent&) {
        wxAutoBufferedPaintDC dc(this);
        dc.SetBackground(*wxWHITE_BRUSH);
        dc.Clear();

        std::set<std::pair<int, int>> pathEdges;
        std::map<int, wxString> order;  
        for (std::size_t i = 0; i < m_path.size(); ++i) {
            if (i + 1 < m_path.size()) pathEdges.insert({m_path[i], m_path[i + 1]});
            wxString& o = order[m_path[i]];
            if (!o.empty()) o += ",";
            o += wxString::Format("%d", (int)i + 1);
        }

        auto edges = m_graph.get_edges();
        for (const auto& k : edges) {
            int a = k.GetFirst(), b = k.GetSecond();
            bool onPath = pathEdges.count({a, b}) > 0;
            dc.SetPen(wxPen(onPath ? *wxRED : *wxBLACK, onPath ? 3 : 1));
            wxPoint s, e;
            EdgeEnds(a, b, s, e);
            DrawArrow(dc, s, e);
            dc.SetTextForeground(onPath ? *wxRED : wxColour(0, 0, 160));
            dc.DrawText(U8(m_graph.get_weight_edge(a, b).c_str()),
                        (s.x + e.x) / 2 + 3, (s.y + e.y) / 2 - 16);
        }

        for (const auto& [id, pos] : m_pos) {
            bool sel = (id == m_from);
            dc.SetPen(*wxBLACK_PEN);
            dc.SetBrush(sel ? wxBrush(wxColour(255, 220, 120)) : wxBrush(wxColour(200, 225, 255)));
            dc.DrawCircle(pos, R);
            dc.SetTextForeground(*wxBLACK);
            wxString t = U8(m_graph.get_vertex_data(id).c_str());
            wxSize ts = dc.GetTextExtent(t);
            dc.DrawText(t, pos.x - ts.x / 2, pos.y - ts.y / 2);

            auto it = order.find(id);
            if (it != order.end()) {
                dc.SetTextForeground(*wxRED);
                dc.DrawText("#" + it->second, pos.x - R, pos.y + R + 2);
            }
        }
    }
};

class StringGraphPanel : public wxPanel {
public:
    explicit StringGraphPanel(wxWindow* parent) : wxPanel(parent) {
        auto* sizer = new wxBoxSizer(wxVERTICAL);

        wxArrayString modes;
        modes.Add(U8("Вершина"));
        modes.Add(U8("Ребро"));
        modes.Add(U8("Двигать"));
        modes.Add(U8("Удалить"));
        modes.Add(U8("Обход (клики)"));
        auto* radio = new wxRadioBox(this, wxID_ANY, U8("Режим"), wxDefaultPosition,
                                     wxDefaultSize, modes, 5, wxRA_SPECIFY_COLS);
        m_canvas = new StringGraphCanvas(this);

        m_pathLabel = new wxStaticText(this, wxID_ANY, U8("Обход: (пусто)"));

        m_input = new wxTextCtrl(this, wxID_ANY);
        m_input->SetHint(U8("Обход по именам вершин, например: A B C"));
        auto* setBtn = new wxButton(this, wxID_ANY, U8("Задать обход"));
        auto* buildBtn = new wxButton(this, wxID_ANY, U8("Собрать строку"));
        auto* clearBtn = new wxButton(this, wxID_ANY, U8("Сбросить обход"));

        auto* row = new wxBoxSizer(wxHORIZONTAL);
        row->Add(m_input, 1, wxRIGHT | wxALIGN_CENTER_VERTICAL, 4);
        row->Add(setBtn, 0, wxRIGHT, 4);
        row->Add(buildBtn, 0, wxRIGHT, 4);
        row->Add(clearBtn, 0);

        m_result = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition,
                                  wxSize(-1, 60), wxTE_MULTILINE | wxTE_READONLY);

        sizer->Add(radio, 0, wxEXPAND | wxALL, 4);
        m_hint = new wxStaticText(this, wxID_ANY,
                                  U8("Клик по пустому месту — новая вершина (имена по алфавиту)"));
        sizer->Add(m_hint, 0, wxLEFT | wxRIGHT, 6);
        sizer->Add(m_canvas, 1, wxEXPAND);
        sizer->Add(m_pathLabel, 0, wxEXPAND | wxALL, 4);
        sizer->Add(row, 0, wxEXPAND | wxLEFT | wxRIGHT, 4);
        sizer->Add(new wxStaticText(this, wxID_ANY, U8("Результат (get_string_from_path):")),
                   0, wxLEFT | wxTOP, 4);
        sizer->Add(m_result, 0, wxEXPAND | wxALL, 4);
        SetSizer(sizer);

        m_canvas->onPathChanged = [this] {
            wxString t = m_canvas->PathText();
            m_pathLabel->SetLabel(U8("Обход: ") + (t.empty() ? U8("(пусто)") : t));
        };

        radio->Bind(wxEVT_RADIOBOX, [this, radio](wxCommandEvent&) {
            static const char* hints[] = {
                "Клик по пустому месту — новая вершина (имена по алфавиту)",
                "Клик по вершине-источнику, затем по приёмнику, затем ввод строки-веса",
                "Перетаскивайте вершины мышью",
                "Клик по вершине или ребру — удалить",
                "Кликайте по вершинам в порядке обхода (можно повторять вершины)"};
            int i = radio->GetSelection();
            m_canvas->SetMode(static_cast<Mode>(i));
            m_hint->SetLabel(U8(hints[i]));
        });

        setBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
            std::string text = m_input->GetValue().utf8_string();
            for (char& c : text)
                if (c == ',' || c == '-' || c == '>') c = ' ';
            std::istringstream is(text);
            std::vector<std::string> labels;
            std::string t;
            while (is >> t) {
                for (char& c : t) c = (char)std::toupper((unsigned char)c);
                labels.push_back(t);
            }
            if (labels.empty()) {
                wxMessageBox(U8("Введите имена вершин через пробел"), U8("Обход"));
                return;
            }
            std::string bad;
            if (!m_canvas->SetPathByLabels(labels, bad))
                wxMessageBox(U8("Нет вершины: ") + U8(bad.c_str()), U8("Обход"), wxICON_WARNING);
        });

        buildBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
            if (m_canvas->PathLength() < 2) {
                m_result->SetValue(U8("В обходе должно быть минимум 2 вершины"));
                return;
            }
            std::string out;
            if (m_canvas->BuildString(out))
                m_result->SetValue(U8(out.c_str()));
            else
                m_result->SetValue(U8("Обход некорректен: между какими-то соседними вершинами нет ребра"));
        });

        clearBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
            m_canvas->ClearPath();
            m_result->Clear();
        });
    }

private:
    StringGraphCanvas* m_canvas;
    wxStaticText* m_hint;
    wxStaticText* m_pathLabel;
    wxTextCtrl* m_input;
    wxTextCtrl* m_result;
};


class IntGraphPanel : public wxPanel {
public:
    explicit IntGraphPanel(wxWindow* parent) : wxPanel(parent) {
        auto* sizer = new wxBoxSizer(wxVERTICAL);

        wxArrayString modes;
        modes.Add(U8("Вершина"));
        modes.Add(U8("Ребро"));
        modes.Add(U8("Двигать"));
        modes.Add(U8("Удалить"));
        modes.Add(U8("Путь (Дейкстра)"));
        auto* radio = new wxRadioBox(this, wxID_ANY, U8("Режим"), wxDefaultPosition,
                                     wxDefaultSize, modes, 5, wxRA_SPECIFY_COLS);
        m_hint = new wxStaticText(this, wxID_ANY, U8("Клик по пустому месту — новая вершина"));
        m_canvas = new GraphCanvas(this);

        sizer->Add(radio, 0, wxEXPAND | wxALL, 4);
        sizer->Add(m_hint, 0, wxLEFT | wxRIGHT, 6);
        sizer->Add(m_canvas, 1, wxEXPAND);
        SetSizer(sizer);

        radio->Bind(wxEVT_RADIOBOX, [this, radio](wxCommandEvent&) {
            static const char* hints[] = {
                "Клик по пустому месту — новая вершина",
                "Клик по вершине-источнику, затем по вершине-приёмнику",
                "Перетаскивайте вершины мышью",
                "Клик по вершине или ребру — удалить",
                "Первый клик — старт, следующие клики — куда строить путь"};
            int i = radio->GetSelection();
            m_canvas->SetMode(static_cast<Mode>(i));
            m_hint->SetLabel(U8(hints[i]));
        });
    }

private:
    GraphCanvas* m_canvas;
    wxStaticText* m_hint;
};

class MainFrame : public wxFrame {
public:
    MainFrame() : wxFrame(nullptr, wxID_ANY, U8("Графы"), wxDefaultPosition, wxSize(950, 760)) {
        auto* book = new wxNotebook(this, wxID_ANY);
        book->AddPage(new IntGraphPanel(book), U8("Граф (числа, Дейкстра)"), true);
        book->AddPage(new StringGraphPanel(book), U8("Граф со строками"));

        auto* sizer = new wxBoxSizer(wxVERTICAL);
        sizer->Add(book, 1, wxEXPAND);
        SetSizer(sizer);

        SetMinSize(wxSize(700, 550));
        Centre();
    }
};

static void Log(const char* msg) {
    std::ofstream f("graph_gui_log.txt", std::ios::app);
    f << msg << "\n";
}

class GraphApp : public wxApp {
public:
    bool OnInit() override {
        Log("OnInit: start");
        try {
            auto* frame = new MainFrame();
            SetTopWindow(frame);
            frame->Show(true);
            frame->Raise();
            Log("OnInit: frame shown");
        } catch (const std::exception& e) {
            Log(e.what());
            wxMessageBox(wxString::FromUTF8(e.what()), wxString::FromUTF8("Ошибка запуска"), wxICON_ERROR);
            return false;
        }
        return true;
    }

    int OnExit() override {
        Log("OnExit");
        return 0;
    }

    bool OnExceptionInMainLoop() override {
        try { throw; }
        catch (const std::exception& e) {
            Log(e.what());
            wxMessageBox(wxString::FromUTF8(e.what()), wxString::FromUTF8("Ошибка"), wxICON_ERROR);
        }
        return true; 
    }
};

#if defined(_MSC_VER)
wxIMPLEMENT_APP(GraphApp);
#else
wxIMPLEMENT_APP_NO_MAIN(GraphApp);

int main(int argc, char** argv) {
    Log("main: start");
    return wxEntry(argc, argv);
}
#endif