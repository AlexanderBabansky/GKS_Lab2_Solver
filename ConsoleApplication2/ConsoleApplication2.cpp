/*51 - вимоги до програм
53 - завдання
49 - правила портфелей
45 - Джонсона-Белмана
39 - оптимізація
*/
#include <iostream>
#include <fstream>
#include "GKS.h"
#include "GUI.h"

#define CHECK_ARG(a) if (!a)throw std::runtime_error("Argument not found")
using namespace std;

std::vector<int> ReadMatrixFromFile(const char* filename) {
    std::ifstream matrix_file;
    matrix_file.open(filename);
    if (!matrix_file.is_open())throw std::runtime_error("No file");
    std::vector<int> temp_matrix;
    std::string read_val;
    while (!matrix_file.eof()) {
        matrix_file >> read_val;
        int v = atoi(read_val.c_str());
        temp_matrix.push_back(v);
    }
    matrix_file.close();
    return temp_matrix;
}

const char* GetArg(const char* flag, int argc, const char** argv) {
    for (int a = 0; a < argc-1; a++) {
        if (strcmp(argv[a], flag) == 0) {
            return argv[a+1];
        }
    }
    return nullptr;
}

bool CheckArgFlag(const char* flag, int argc, const char** argv) {
    for (int a = 0; a < argc; a++) {
        if (strcmp(argv[a], flag) == 0) {
            return true;
        }
    }
    return false;
}

int main(int argc, const char** argv)
{
    if (argc < 5) {
        std::cout <<"\
-m\toperations count;\n\
-n\tdetails count;\n\
-t\tprocess time matrix file;\n\
-tm\tpipeline matrix\n\
-r\tresult file (def: result.txt);\n\
-mode\tjb|jbi|sim\n\
-br\tbag rule\n\
-g\tgui";
        return 0;
    }
    int m=0, n=0;
    const char* t_filename = nullptr;
    const char* res_filename = nullptr;
    const char* mode = nullptr;
    int bag_rule_id = -1;
    bool show_gui = false;
    std::vector<int> tm_matrix;

    try {
        const char* temp = GetArg("-m", argc, argv);
        CHECK_ARG(temp);
        m = atoi(temp);
        temp = GetArg("-n", argc, argv);
        CHECK_ARG(temp);
        n = atoi(temp);
        t_filename = GetArg("-t", argc, argv);
        CHECK_ARG(t_filename);        
        auto t_matrix = ReadMatrixFromFile(t_filename);
        t_filename = GetArg("-tm", argc, argv);
        if (t_filename) {
            tm_matrix = ReadMatrixFromFile(t_filename);
        }

        res_filename = GetArg("-r", argc, argv);        
        if (!res_filename)
            res_filename = "result.txt";
        show_gui = CheckArgFlag("-g", argc, argv);
        mode = GetArg("-mode", argc, argv);
        CHECK_ARG(mode);


        GSKSolver solver;
        solver.InitData(m,n,t_matrix,tm_matrix);

        Timetable tt;
        if (strcmp(mode, "jb") == 0) {
            tt = solver.solve_johnson();
        }else if (strcmp(mode, "jbi") == 0) {
            tt = solver.solve_johnson(true);
        }
        else if (strcmp(mode, "sim") == 0) {
            temp = GetArg("-br", argc, argv);
            CHECK_ARG(temp);
            bag_rule_id = atoi(temp);
            tt = solver.solve_simulation(bag_rule_id-1);
        }
        else {
            throw runtime_error("Invalid mode");
        }
        
        auto tt_m = tt.GetTimetableMatrix();

        ofstream res_file;
        res_file.open(res_filename, ios_base::trunc | ios_base::out);
        if (!res_file.is_open())throw runtime_error("Could not open result file");
        res_file << "P matrix:" << endl << endl;
        for (int i = 0; i < n; i++) {
            for (auto& j : tt_m) {
                res_file << j[i] << "\t";
            }
            res_file << endl;
        }

        res_file << endl;
        res_file << "Quality attributes:" << endl <<
            "1.2: " << tt.ComputeQuality(12) << endl <<
            "2.2: " << tt.ComputeQuality(22) << endl <<
            "2.4: " << tt.ComputeQuality(24) << endl <<
            "2.7: " << tt.ComputeQuality(27) << endl <<
            "3.2: " << tt.ComputeQuality(32) << endl <<
            "3.5: " << tt.ComputeQuality(35) << endl <<
            "3.7: " << tt.ComputeQuality(37) << endl;
        res_file.close();

        if (show_gui) {
            GUIForm gui(&tt);
            gui.Show();
        }
    }
    catch (std::exception& e) {
        cout << "Error: " << e.what() << endl;
    }
    return 0;
}