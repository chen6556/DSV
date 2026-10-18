#include <filesystem>
#include <fstream>
#include "RegressionTest.hpp"
#include "Utils.hpp"
#include "../src/base/Graph.hpp"
#include "../src/base/UndoStack.hpp"
#include "../src/io/DSVReaderWriter.hpp"


static bool all_passed = true;

void RegressionTest::run_tests(bool &success)
{
    test_dsv_round_trip();
    test_layer_group_operations();
    PASSED
}

void RegressionTest::test_dsv_round_trip()
{
    Graph original;
    original.append_group("LayerA");
    original.append_group("LayerB");

    original.container_group(0).name = "LayerA";
    original.container_group(1).name = "LayerB";

    Geo::Point *point = new Geo::Point(1.5, -2.5);
    Geo::Polyline *polyline = new Geo::Polyline({Geo::Point(0.0, 0.0), Geo::Point(2.0, 0.0), Geo::Point(2.0, 2.0)});

    original.append(point, 0);
    original.append(polyline, 1);

    const std::filesystem::path temp_path = std::filesystem::temp_directory_path() / "dsv_roundtrip_regression.dsv";
    std::ofstream output(temp_path, std::ios::out | std::ios::trunc);
    DSVReaderWriter writer(&original);
    writer.write(output);
    output.close();

    Graph restored;
    std::ifstream input(temp_path, std::ios::in);
    DSVReaderWriter reader(&restored);
    reader.read(input);
    input.close();

    TEST(restored.size() == 2, "DSV round trip keeps the original layer count")
    TEST(restored.container_group(0).size() == 1, "LayerA keeps a single object")
    TEST(restored.container_group(1).size() == 1, "LayerB keeps a single polyline")

    const Geo::Point &restored_point = *static_cast<const Geo::Point *>(restored.container_group(0)[0]);
    TEST(fequal(1.5, restored_point.x) && fequal(-2.5, restored_point.y), "Point coordinates survive DSV round trip")

    const Geo::Polyline &restored_polyline = *static_cast<const Geo::Polyline *>(restored.container_group(1)[0]);
    TEST(restored_polyline.size() == 3, "Polyline vertex count survives DSV round trip")
    TEST(fequal(0.0, restored_polyline[0].x) && fequal(0.0, restored_polyline[0].y), "Polyline start point is preserved")
    TEST(fequal(2.0, restored_polyline[1].x) && fequal(0.0, restored_polyline[1].y), "Polyline middle point is preserved")
    TEST(fequal(2.0, restored_polyline[2].x) && fequal(2.0, restored_polyline[2].y), "Polyline end point is preserved")
}

void RegressionTest::test_layer_group_operations()
{
    Graph graph;
    graph.append_group("Base");
    graph.append_group("Overlay");

    graph.container_group(0).append(new Geo::Point(0.0, 0.0));
    graph.container_group(1).append(new Geo::Point(1.0, 1.0));

    graph.insert_group(1);
    TEST(graph.size() == 3, "Insert group grows graph by one layer")

    graph.remove_group(1);
    TEST(graph.size() == 2, "Remove group shrinks graph back to original size")

    graph.container_group(0).append(new Geo::Point(2.0, 2.0));
    TEST(graph.container_group(0).size() == 2, "Appended object remains in the selected layer")

    graph.container_group(0).clear();
    TEST(graph.container_group(0).empty(), "Clearing a layer removes its objects")
}
