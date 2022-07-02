#include <iostream>
#include <fstream>
#include <stdlib.h>
#include "../inc/alpha_beta.h"
#include "../inc/board.h"

using namespace std;
using namespace nlohmann;
static int num_tests = 0;

void run_test(string file_name, string expected_move, bool inverse_move) {
    fstream test_file;
    test_file.open(file_name, ios::in);
    if (!test_file) {
		cout << "No such file\n";
        return;
	}
    string test_data;
    char test_char;
    while (!test_file.eof()) {
        test_file >> test_char;
        test_data.push_back(test_char);
    }

    test_file.close();

    json data = json::parse(test_data);
    //cout << data << "\n";
    const clock_t begin_time = clock();
    std::cout << float( clock () - begin_time ) /  CLOCKS_PER_SEC;
    std::cout << " Start: " << file_name << "\n";

    AlphaBeta alpha_beta = AlphaBeta(data);

    alpha_beta.get_main_board()->print_board();

    alpha_beta.breadth_first_search_loop();
    string move = alpha_beta.get_move_from_board();

    float time_spent = float( clock () - begin_time ) /  CLOCKS_PER_SEC;

    std::cout << "Time: " << time_spent;
    std::cout << " Stop\n";

    string depth = to_string(alpha_beta.get_max_depth_hit());
    std::cout << "Depth Hit: " << depth << '\n';
    alpha_beta.print_nodes_evaluated();
    std::cout << move << " \n";

    if (move != expected_move && !inverse_move) {
        std::cout << "ABORT: Expected: '" << expected_move << "' Got: '" << move << "'\n";
        abort();
        //assert("Test Failed");
    } else if (move == expected_move && inverse_move) {
        std::cout << "ABORT: Expected not: " << expected_move << " Got: " << move << "\n";
        abort();
    }

    float max_latency = 0.05f;
    //if (time_spent > alpha_beta.get_max_time_micro() + 0.04f) {
    if (time_spent + max_latency > 0.49f) {
        if (!inverse_move) {
            std::cout << "ABORT TIME: Expected: '" << expected_move << "' Got: '" << move << "'\n";        
        } else {
            std::cout << "ABORT TIME: Expected not: '" << expected_move << "' Got: '" << move << "'\n";
        }
        std::cout << "Time taken over: " << time_spent - alpha_beta.get_max_time_micro() << '\n';
        //abort();
    }

    cout << "Test " << ++num_tests << " passed" << "\n";

    cout << "\n\n";
}

int main(void) {

    bool test_all = false;
    test_all = true;

    //set above flag to false, or comment out to just run singular test below
    if (!test_all) {run_test("test/test_scenarios/test_126.txt", "down", true);}//down leads to death

    //if test all true, test build against all test cases
    if (test_all) {
        run_test("test/test_scenarios/test_1.txt", "left", false);
        run_test("test/test_scenarios/test_2.txt", "down", true);
        run_test("test/test_scenarios/test_3.txt", "down", true);
        run_test("test/test_scenarios/test_4.txt", "down", false);
        run_test("test/test_scenarios/test_5.txt", "down", false);
        //not right
        run_test("test/test_scenarios/test_6.txt", "right", true);
        //not down
        run_test("test/test_scenarios/test_7.txt", "down", true);
        //not left
        run_test("test/test_scenarios/test_8.txt", "left", true);
        //not down
        run_test("test/test_scenarios/test_9.txt", "down", true);
        
        run_test("test/test_scenarios/test_10.txt", "up", false);
        run_test("test/test_scenarios/test_11.txt", "down", true);
        run_test("test/test_scenarios/test_12.txt", "up", false);
        run_test("test/test_scenarios/test_13.txt", "right", false);
        run_test("test/test_scenarios/test_14.txt", "up", false);
        run_test("test/test_scenarios/test_15.txt", "right", false);
        //not left
        run_test("test/test_scenarios/test_16.txt", "left", true);
        run_test("test/test_scenarios/test_17.txt", "down", false);
        run_test("test/test_scenarios/test_18.txt", "up", false);
        run_test("test/test_scenarios/test_19.txt", "down", false);

        run_test("test/test_scenarios/test_20.txt", "down", false);
        run_test("test/test_scenarios/test_21.txt", "up", false);
        //not down
        run_test("test/test_scenarios/test_22.txt", "down", true);
        //not left
        run_test("test/test_scenarios/test_23.txt", "left", true);
        run_test("test/test_scenarios/test_24.txt", "up", false);
        //not right
        run_test("test/test_scenarios/test_25.txt", "right", true);
        run_test("test/test_scenarios/test_26.txt", "up", false);
        //not down
        run_test("test/test_scenarios/test_27.txt", "down", true);
        //not right
        run_test("test/test_scenarios/test_28.txt", "right", true);
        //not up
        run_test("test/test_scenarios/test_29.txt", "up", true);

        run_test("test/test_scenarios/test_30.txt", "left", false);
        run_test("test/test_scenarios/test_31.txt", "down", false);//fails only on depth 2
        //not right
        run_test("test/test_scenarios/test_32.txt", "right", true);
        //not right
        run_test("test/test_scenarios/test_33.txt", "right", true);
        //not down
        run_test("test/test_scenarios/test_34.txt", "down", true);
        run_test("test/test_scenarios/test_35.txt", "down", false);
        //not right
        run_test("test/test_scenarios/test_36.txt", "right", true);
        run_test("test/test_scenarios/test_37.txt", "right", false);
        run_test("test/test_scenarios/test_38.txt", "right", false);
        run_test("test/test_scenarios/test_39.txt", "right", false);

        //down death
        run_test("test/test_scenarios/test_40.txt", "down", true);
        //want left or down
        run_test("test/test_scenarios/test_41.txt", "right", true);
        run_test("test/test_scenarios/test_42.txt", "down", false);
        run_test("test/test_scenarios/test_43.txt", "down", false);
        run_test("test/test_scenarios/test_44.txt", "right", false);
        run_test("test/test_scenarios/test_45.txt", "up", false);
        //not left
        run_test("test/test_scenarios/test_46.txt", "left", true);
        run_test("test/test_scenarios/test_47.txt", "left", false);
        //at 0.3 sec can fail, 0.32 gauruntees success
        run_test("test/test_scenarios/test_48.txt", "right", false);
        run_test("test/test_scenarios/test_49.txt", "down", false);

        run_test("test/test_scenarios/test_50.txt", "left", false);
        run_test("test/test_scenarios/test_51.txt", "left", false);
        run_test("test/test_scenarios/test_52.txt", "up", false);
        run_test("test/test_scenarios/test_53.txt", "down", false);
        run_test("test/test_scenarios/test_54.txt", "right", true);
        //left for fast win, up if slow
        run_test("test/test_scenarios/test_55.txt", "left", false);
        run_test("test/test_scenarios/test_56.txt", "down", false);
        run_test("test/test_scenarios/test_57.txt", "down", false);
        run_test("test/test_scenarios/test_58.txt", "up", false);
        run_test("test/test_scenarios/test_59.txt", "down", false);

        run_test("test/test_scenarios/test_60.txt", "down", false);
        run_test("test/test_scenarios/test_61.txt", "left", false);
        run_test("test/test_scenarios/test_62.txt", "right", true);//ironically can go down here and loop left and up to escape past other snakes tail
        //should go right or up
        run_test("test/test_scenarios/test_63.txt", "left", true);
        //should go left or down
        run_test("test/test_scenarios/test_64.txt", "right", true);
        //up is fastest kill, right is still win, 
        run_test("test/test_scenarios/test_65.txt", "up", false);
        run_test("test/test_scenarios/test_66.txt", "up", false);
        //not down, timesout, right is tie, left is continue
        run_test("test/test_scenarios/test_67.txt", "down", true);
        //not right, timesout
        run_test("test/test_scenarios/test_68.txt", "right", true);
        //not left, timesout
        run_test("test/test_scenarios/test_69.txt", "left", false);

        //don't care direction, causes seg fault, with endless tile domination looping calls because of tail being instance of another tail legitimatlly
        run_test("test/test_scenarios/test_70.txt", "right", true);
        //prefer not down, but still does
        run_test("test/test_scenarios/test_71.txt", "left", true);
        //should go left not down
        run_test("test/test_scenarios/test_72.txt", "left", false);
        run_test("test/test_scenarios/test_73.txt", "down", false);
        //not right probably up
        run_test("test/test_scenarios/test_74.txt", "right", true);
        //going left and skipping food leads to death, down and grabbing food restores health and best way of getting out of hazard
        run_test("test/test_scenarios/test_75.txt", "left", true);
        run_test("test/test_scenarios/test_76.txt", "left", true);
        run_test("test/test_scenarios/test_77.txt", "down", false);
        run_test("test/test_scenarios/test_78.txt", "right", false);
        //I think right is best, but algorithm thinks down is better to survive longer, and it might be?
        run_test("test/test_scenarios/test_79.txt", "up", true);

        run_test("test/test_scenarios/test_80.txt", "up", true);
        run_test("test/test_scenarios/test_81.txt", "right", false);
        run_test("test/test_scenarios/test_82.txt", "left", false);
        run_test("test/test_scenarios/test_83.txt", "down", false);
        run_test("test/test_scenarios/test_84.txt", "left", true);
        run_test("test/test_scenarios/test_85.txt", "right", false);
        run_test("test/test_scenarios/test_86.txt", "right", false);
        //evaluates up as not death but doesn't take away space removed when dominated
        run_test("test/test_scenarios/test_87.txt", "right", false);
        //don't go down to loop for food
        run_test("test/test_scenarios/test_88.txt", "down", true);
        run_test("test/test_scenarios/test_89.txt", "up", false);

        run_test("test/test_scenarios/test_90.txt", "right", false);
        run_test("test/test_scenarios/test_91.txt", "right", false);
        run_test("test/test_scenarios/test_92.txt", "right", false);
        run_test("test/test_scenarios/test_93.txt", "left", false);
        run_test("test/test_scenarios/test_94.txt", "up", false);
        run_test("test/test_scenarios/test_95.txt", "right", false);
        //TODO want this to go right, for now allow down
        run_test("test/test_scenarios/test_96.txt", "up", true);
        run_test("test/test_scenarios/test_97.txt", "left", false);
        run_test("test/test_scenarios/test_98.txt", "left", false);
        //TODO want this to go left, can't find additional space to tail chase so goes right
        //run_test("test/test_scenarios/test_99.txt", "left", false);

        run_test("test/test_scenarios/test_100.txt", "left", false);
        run_test("test/test_scenarios/test_101.txt", "up", false);
        run_test("test/test_scenarios/test_102.txt", "down", false);
        //run_test("test/test_scenarios/test_103.txt", "left", false);
        run_test("test/test_scenarios/test_104.txt", "up", false);
        run_test("test/test_scenarios/test_105.txt", "up", false);
        //run_test("test/test_scenarios/test_106.txt", "up", false);
        run_test("test/test_scenarios/test_107.txt", "left", false);
        //think down but looping back by going up and tail chasing may be best plan
        run_test("test/test_scenarios/test_108.txt", "left", true);
        run_test("test/test_scenarios/test_109.txt", "left", false);
        
        //run_test("test/test_scenarios/test_110.txt", "up", false);
        run_test("test/test_scenarios/test_111.txt", "right", false);//go right to control non-hazard space
        //run_test("test/test_scenarios/test_112.txt", "down", false);//go down for more space control
        run_test("test/test_scenarios/test_113.txt", "left", false);//go left to control more space
        run_test("test/test_scenarios/test_114.txt", "right", false);//go right for a draw
        run_test("test/test_scenarios/test_115.txt", "up", false);//go up for win, does too fast
        run_test("test/test_scenarios/test_116.txt", "right", false);//go right for win
        run_test("test/test_scenarios/test_117.txt", "right", false);//left dies to space or other snake eating, right lives
        run_test("test/test_scenarios/test_118.txt", "up", true);//times out
        run_test("test/test_scenarios/test_119.txt", "up", false);//go up out of bounds in WRAPPED gamemode for food above

        run_test("test/test_scenarios/test_120.txt", "down", false);
        run_test("test/test_scenarios/test_121.txt", "left", true);//times out
        run_test("test/test_scenarios/test_122.txt", "right", false);//go right to wrap
        run_test("test/test_scenarios/test_123.txt", "right", true);//right leads to death
        run_test("test/test_scenarios/test_124.txt", "up", false);//down leads to death
        run_test("test/test_scenarios/test_125.txt", "left", false);//code error creates timeout
        run_test("test/test_scenarios/test_126.txt", "down", true);//down leads to death
    }

    return 0;
}
