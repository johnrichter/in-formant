// Checks of the documents, the help pages, the fonts, and the release rules. The tests read the files of the
// source folder. INFORMANT_SOURCE_DIR is the root of the repository.
#include <gtest/gtest.h>
#include "../src/gui/uiscale.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

const fs::path root = INFORMANT_SOURCE_DIR;

std::string readFile(const fs::path &path)
{
    std::ifstream in(path, std::ios::binary);
    EXPECT_TRUE(in.good()) << "cannot read " << path;
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

std::vector<std::string> readLines(const fs::path &path)
{
    std::vector<std::string> lines;
    std::istringstream in(readFile(path));
    for (std::string line; std::getline(in, line);) lines.push_back(line);
    return lines;
}

// The Markdown files of the docs folder, by file name.
std::vector<fs::path> docFiles()
{
    std::vector<fs::path> files;
    for (const auto &entry : fs::directory_iterator(root / "docs")) {
        if (entry.path().extension() == ".md") files.push_back(entry.path());
    }
    std::sort(files.begin(), files.end());
    return files;
}

// The pages for developers are not in the Help window.
bool isDeveloperPage(const fs::path &path)
{
    const std::string name = path.filename().string();
    return name == "test-plan.md" || name == "design-system.md";
}

std::string firstHeading(const fs::path &path)
{
    for (const std::string &line : readLines(path)) {
        if (line.rfind("# ", 0) == 0) return line.substr(2);
    }
    return {};
}

struct HelpPage {
    std::string title;
    std::string file;
};

// The pages that the Help window lists, from the file HelpWindow.qml.
std::vector<HelpPage> helpWindowPages()
{
    std::vector<HelpPage> pages;
    const std::string text = readFile(root / "src/qml/HelpWindow.qml");
    const std::regex entry(R"re(\{\s*title:\s*"([^"]+)",\s*file:\s*"([^"]+)"\s*\})re");
    for (auto it = std::sregex_iterator(text.begin(), text.end(), entry); it != std::sregex_iterator(); ++it) {
        pages.push_back({ (*it)[1], (*it)[2] });
    }
    return pages;
}

// The text of a command. The command writes to a pipe, and the function returns its output.
std::string runCommand(const std::string &command, int *status = nullptr)
{
    std::string output;
    FILE *pipe = popen(command.c_str(), "r");
    if (!pipe) {
        if (status) *status = -1;
        return output;
    }
    char buffer[512];
    while (fgets(buffer, sizeof buffer, pipe)) output += buffer;
    const int code = pclose(pipe);
    if (status) *status = code;
    return output;
}

// The style check of the writing rules. The variable INFORMANT_STE_CHECK gives the path of the script. The test
// runs only when the variable is set, so the result does not depend on the files of one machine.
fs::path styleChecker()
{
    if (const char *given = std::getenv("INFORMANT_STE_CHECK")) return given;
    return {};
}

// The number of violations that the style check reports for a file. 0 means that the file is clean.
int styleViolations(const fs::path &tool, const fs::path &file)
{
    const std::string out = runCommand("python3 -I \"" + tool.string() + "\" \"" + file.string() + "\" 2>&1");
    if (out.find("CLEAN") != std::string::npos) return 0;
    const std::regex found(R"(VIOLATIONS FOUND: (\d+))");
    std::smatch m;
    if (std::regex_search(out, m, found)) return std::stoi(m[1]);
    ADD_FAILURE() << "unknown output of the style check for " << file << ": " << out;
    return -1;
}

std::vector<std::string> linksIn(const std::string &text)
{
    std::vector<std::string> links;
    const std::regex url(R"(https?://[^\s)>\]"`]+)");
    for (auto it = std::sregex_iterator(text.begin(), text.end(), url); it != std::sregex_iterator(); ++it) {
        std::string link = (*it)[0];
        while (!link.empty() && std::string(".,;:").find(link.back()) != std::string::npos) link.pop_back();
        links.push_back(link);
    }
    return links;
}

std::vector<fs::path> prosePaths()
{
    std::vector<fs::path> files = docFiles();
    files.push_back(root / "README.md");
    return files;
}

}

// The writing rules apply to each file of docs. The README has known violations, and it must not get more.
TEST(Docs, TP_DOC_01_EachDocumentFollowsTheWritingRules)
{
    const fs::path tool = styleChecker();
    if (tool.empty()) GTEST_SKIP() << "Set INFORMANT_STE_CHECK to the path of the style check script.";

    for (const fs::path &file : docFiles()) {
        EXPECT_EQ(styleViolations(tool, file), 0) << file.filename();
    }
    constexpr int readmeKnownViolations = 12;
    EXPECT_LE(styleViolations(tool, root / "README.md"), readmeKnownViolations);
}

// Each link is a full https address. A test that goes to the network is in the next case.
TEST(Docs, TP_DOC_02_EachLinkIsAnHttpsAddress)
{
    for (const fs::path &file : prosePaths()) {
        for (const std::string &link : linksIn(readFile(file))) {
            EXPECT_EQ(link.rfind("https://", 0), 0u) << file.filename() << ": " << link;
            EXPECT_EQ(link.find(' '), std::string::npos) << link;
            EXPECT_GT(link.size(), std::string("https://a.b").size()) << link;
        }
    }
}

// Each source address answers with 200. This test needs the network, so it runs only when the variable
// INFORMANT_CHECK_LINKS is set.
TEST(Docs, TP_DOC_02_EachSourceAddressAnswersWith200)
{
    if (!std::getenv("INFORMANT_CHECK_LINKS")) GTEST_SKIP() << "Set INFORMANT_CHECK_LINKS=1 to test the links on the network.";

    std::set<std::string> links;
    for (const fs::path &file : prosePaths()) {
        for (const std::string &link : linksIn(readFile(file))) links.insert(link);
    }
    ASSERT_FALSE(links.empty());
    for (const std::string &link : links) {
        const std::string code = runCommand("curl -s -o /dev/null -w '%{http_code}' -L --max-time 20 \"" + link + "\"");
        // A publisher can refuse an automated client (401 or 403), and a server can fail for a short time (5xx).
        // These answers do not show that the link is dead, so the test only reports them.
        if (code == "401" || code == "403" || (code.size() == 3 && code[0] == '5')) {
            std::printf("[ NOTE ] %s answered %s. Check this link by hand.\n", link.c_str(), code.c_str());
            continue;
        }
        EXPECT_EQ(code, "200") << link;
    }
}

// Each help page is in the resources, in the list of the Help window, and in the README.
TEST(Docs, TP_DOC_05_EachHelpPageIsInTheResourcesTheHelpWindowAndTheReadme)
{
    const std::string qrc = readFile(root / "resources/qml.qrc");
    const std::string readme = readFile(root / "README.md");
    const std::vector<HelpPage> pages = helpWindowPages();
    ASSERT_FALSE(pages.empty());

    std::set<std::string> listed;
    for (const HelpPage &page : pages) listed.insert(page.file);
    EXPECT_EQ(listed.size(), pages.size()) << "a page is in the list two times";

    std::set<std::string> found;
    for (const fs::path &file : docFiles()) {
        if (isDeveloperPage(file)) continue;
        const std::string stem = file.stem().string();
        found.insert(stem);
        const std::string name = file.filename().string();
        EXPECT_NE(qrc.find("alias=\"help/" + name + "\">../docs/" + name), std::string::npos) << name << " is not in qml.qrc";
        EXPECT_TRUE(listed.count(stem)) << name << " is not in the list of the Help window";
        EXPECT_NE(readme.find("](docs/" + name + ")"), std::string::npos) << name << " has no link in the README";
    }
    for (const HelpPage &page : pages) {
        EXPECT_TRUE(found.count(page.file)) << page.file << " is in the Help window, but it has no file in docs";
    }
}

// The page about warm-ups has the warning at the top and a list of sources.
TEST(Docs, TP_DOC_06_TheWarmUpPageHasTheCautionAndTheSources)
{
    const std::vector<std::string> lines = readLines(root / "docs/warmups-and-vocal-health.md");
    size_t caution = lines.size(), firstSection = lines.size(), sources = lines.size();
    for (size_t i = 0; i < lines.size(); ++i) {
        if (caution == lines.size() && lines[i].rfind("**CAUTION.**", 0) == 0) caution = i;
        if (firstSection == lines.size() && lines[i].rfind("## ", 0) == 0) firstSection = i;
        if (lines[i] == "## Sources") sources = i;
    }
    ASSERT_LT(caution, lines.size());
    EXPECT_LT(caution, firstSection) << "the warning must come before the first section";
    EXPECT_NE(lines[caution].find("not medical advice"), std::string::npos);

    ASSERT_LT(sources, lines.size());
    int entries = 0;
    for (size_t i = sources + 1; i < lines.size(); ++i) {
        EXPECT_NE(lines[i].rfind("## ", 0), 0u) << "the section Sources must be the last section";
        if (lines[i].rfind("- ", 0) == 0 || lines[i].rfind("* ", 0) == 0) ++entries;
    }
    EXPECT_GE(entries, 3);
}

// The worked examples of the guides use the formula: semitones = 12 times log2 of the ratio.
TEST(Docs, TP_DOC_08_TheWorkedExamplesMatchTheFormula)
{
    const std::regex example(R"(P5 of (\d+) Hz and P95 of (\d+) Hz\. The range is 12 times log2 of (\d+) / (\d+), which is ([0-9.]+) semitones)");
    auto semitones = [](double low, double high) { return 12.0 * std::log2(high / low); };

    int count = 0;
    for (const char *name : { "feminization.md", "masculinization.md" }) {
        const std::string text = readFile(root / "docs" / name);
        std::vector<double> exact, shown;
        for (auto it = std::sregex_iterator(text.begin(), text.end(), example); it != std::sregex_iterator(); ++it) {
            const double low = std::stod((*it)[1]), high = std::stod((*it)[2]);
            EXPECT_EQ((*it)[3], (*it)[2]) << name << ": the formula must use the P95 value";
            EXPECT_EQ((*it)[4], (*it)[1]) << name << ": the formula must use the P5 value";
            EXPECT_NEAR(std::stod((*it)[5]), semitones(low, high), 0.05) << name << ": " << (*it)[0];
            exact.push_back(semitones(low, high));
            shown.push_back(std::stod((*it)[5]));
            ++count;
        }
        ASSERT_EQ(exact.size(), 2u) << name;

        // The text says how much wider or narrower the second example is.
        const std::regex difference(R"(is ([0-9.]+) semitones (wider|narrower))");
        std::smatch m;
        ASSERT_TRUE(std::regex_search(text, m, difference)) << name;
        EXPECT_NEAR(std::stod(m[1]), std::fabs(exact[1] - exact[0]), 0.05) << name;
        EXPECT_NEAR(std::stod(m[1]), std::fabs(shown[1] - shown[0]), 0.11) << name;
        EXPECT_EQ(std::string(m[2]), exact[1] > exact[0] ? "wider" : "narrower") << name;
    }
    EXPECT_EQ(count, 4);
}

// The docs, the README, and the strings of the windows use American spelling.
TEST(Docs, TP_DOC_11_NoBritishSpellingInTheDocumentsAndTheStringsOfTheWindows)
{
    const std::regex british(
        R"(\b(\w*colour\w*|\w*centre\w*|centred|practis(e|ed|es|ing)|\w*behaviour\w*|\w*favour\w*|\w*honour\w*|)"
        R"(\w*neighbour\w*|labelled|labelling|modelled|modelling|travell\w+|cancell\w+|grey\w*|analyse|analysed|analysing|)"
        R"(\w*organis\w+|\w*recognis\w+|\w*realis\w+|\w*summaris\w+|\w*optimis\w+|\w*normalis\w+|\w*minimis\w+|)"
        R"(\w*maximis\w+|metres?|litres?|fibres?|theatres?|programmes?|licence\w*|defence|judgement|ageing|whilst|amongst|artefacts?)\b)",
        std::regex::icase);

    for (const fs::path &file : prosePaths()) {
        for (const std::string &line : readLines(file)) {
            // The test plan quotes the words that it forbids.
            if (line.find("British spelling") != std::string::npos) continue;
            std::smatch m;
            EXPECT_FALSE(std::regex_search(line, m, british)) << file.filename() << ": " << m[0];
        }
    }

    // The comments and the names in the source and in the tests use American spelling too. This file lists the
    // words, so it is not in the scan.
    std::vector<fs::path> code;
    for (const char *folder : { "src", "tests", "dist-res" }) {
        for (const auto &entry : fs::recursive_directory_iterator(root / folder)) {
            if (!entry.is_regular_file()) continue;
            const std::string extension = entry.path().extension().string();
            if (extension != ".cpp" && extension != ".h" && extension != ".mm" && extension != ".plist" && extension != ".py") continue;
            if (entry.path().filename() == "docs_test.cpp") continue;
            // The code of the gci folder comes from a paper and has no caller. It leaves the program soon.
            if (entry.path().string().find("/src/analysis/gci/") != std::string::npos) continue;
            code.push_back(entry.path());
        }
    }
    code.push_back(root / "CONTRIBUTING.md");
    EXPECT_GT(code.size(), 100u);
    for (const fs::path &file : code) {
        for (const std::string &line : readLines(file)) {
            std::smatch m;
            EXPECT_FALSE(std::regex_search(line, m, british)) << file.filename() << ": " << m[0];
        }
    }

    // In the windows, only the text in quotes counts. The names of Qt, such as Material.BlueGrey, stay as they are.
    const std::regex quoted(R"re("((?:[^"\\]|\\.)*)")re");
    for (const auto &entry : fs::recursive_directory_iterator(root / "src/qml")) {
        if (entry.path().extension() != ".qml") continue;
        for (const std::string &line : readLines(entry.path())) {
            const size_t comment = line.find("//");
            const std::string code = comment == std::string::npos ? line : line.substr(0, comment);
            for (auto it = std::sregex_iterator(code.begin(), code.end(), quoted); it != std::sregex_iterator(); ++it) {
                std::smatch m;
                const std::string text = (*it)[1];
                EXPECT_FALSE(std::regex_search(text, m, british)) << entry.path().filename() << ": " << m[0];
            }
        }
    }
}

// A page name in the text is the name of a tab of the Help window, or the title of a page.
TEST(Docs, TP_DOC_13_EachPageNameInTheTextMatchesAPageOfTheHelpWindow)
{
    std::set<std::string> names;
    for (const HelpPage &page : helpWindowPages()) {
        names.insert(page.title);
        names.insert(firstHeading(root / "docs" / (page.file + ".md")));
    }
    ASSERT_GE(names.size(), 8u);

    const std::regex reference(R"re(\bpages? "([^"]+)")re");
    int checked = 0;
    for (const fs::path &file : prosePaths()) {
        if (file.filename() == "test-plan.md") continue;
        const std::string text = readFile(file);
        for (auto it = std::sregex_iterator(text.begin(), text.end(), reference); it != std::sregex_iterator(); ++it) {
            EXPECT_TRUE(names.count((*it)[1])) << file.filename() << ": " << (*it)[0];
            ++checked;
        }
    }
    EXPECT_GE(checked, 5);
}

namespace {

std::string titleCase(std::string word)
{
    if (!word.empty()) word[0] = (char) std::toupper((unsigned char) word[0]);
    return word;
}

}

// The design page lists the numbers of the code in src/gui/uiscale.h.
TEST(Docs, TP_DOC_20_TheDesignSystemPageListsTheNumbersOfTheCode)
{
    namespace S = Gui::UiScale;
    const std::string text = readFile(root / "docs/design-system.md");

    // The type scale.
    const std::regex type(R"(- \*\*(\w+), (\d+) pixels, line (\d+)\.\*\*)");
    std::vector<std::array<std::string, 3>> typeRows;
    for (auto it = std::sregex_iterator(text.begin(), text.end(), type); it != std::sregex_iterator(); ++it) {
        typeRows.push_back({ (*it)[1], (*it)[2], (*it)[3] });
    }
    ASSERT_EQ(typeRows.size(), S::typeSteps.size());
    for (size_t i = 0; i < S::typeSteps.size(); ++i) {
        EXPECT_EQ(typeRows[i][0], titleCase(S::typeSteps[i].name));
        EXPECT_EQ(std::stoi(typeRows[i][1]), S::typeSteps[i].size) << S::typeSteps[i].name;
        EXPECT_EQ(std::stoi(typeRows[i][2]), S::typeSteps[i].line) << S::typeSteps[i].name;
    }

    // The spacing scale.
    const std::regex space(R"(- \*\*(xs|sm|md|lg|xl|xxl|xxxl), (\d+)\.\*\*)");
    std::vector<std::pair<std::string, int>> spaceRows;
    for (auto it = std::sregex_iterator(text.begin(), text.end(), space); it != std::sregex_iterator(); ++it) {
        spaceRows.push_back({ (*it)[1], std::stoi((*it)[2]) });
    }
    ASSERT_EQ(spaceRows.size(), S::spaceSteps.size());
    for (size_t i = 0; i < S::spaceSteps.size(); ++i) {
        EXPECT_EQ(spaceRows[i].first, S::spaceSteps[i].name);
        EXPECT_EQ(spaceRows[i].second, S::spaceSteps[i].size) << S::spaceSteps[i].name;
    }

    // The radii, the ratio, the base size, the minimum size, the measure, and the contrast.
    EXPECT_NE(text.find("The radii are " + std::to_string(S::radiusSteps[0].size) + ", " + std::to_string(S::radiusSteps[1].size)
                        + ", and " + std::to_string(S::radiusSteps[2].size) + " pixels."), std::string::npos);
    EXPECT_NE(text.find("same corner radius: " + std::to_string(S::cornerRadius) + " pixels"), std::string::npos);
    EXPECT_NE(text.find("minor third (1.2) from a base of " + std::to_string(S::bodySize) + " pixels"), std::string::npos);
    EXPECT_DOUBLE_EQ(S::typeRatio, 1.2);
    EXPECT_NE(text.find("Nothing in the app is smaller than " + std::to_string(S::minimumSize) + " pixels"), std::string::npos);
    EXPECT_NE(text.find("at most " + std::to_string(S::textMeasure) + " pixels"), std::string::npos);
    EXPECT_NE(text.find("A 4-point grid"), std::string::npos);
    EXPECT_EQ(S::gridUnit, 4);

    // The scale of the help pages.
    using P = S::Prose;
    auto expectText = [&](const std::string &part) { EXPECT_NE(text.find(part), std::string::npos) << part; };
    expectText("**Body.** " + std::to_string(P::bodySize) + " pixels on a line of " + std::to_string(P::bodyLine) + " pixels.");
    expectText("**Headings.** " + std::to_string(P::h1Size) + ", " + std::to_string(P::h2Size) + ", and " + std::to_string(P::h3Size) + " pixels");
    expectText("**Code and tables.** " + std::to_string(P::codeSize) + " pixels on a line of " + std::to_string(P::codeLine));
    expectText(std::to_string(P::paragraphGap) + " pixels below a paragraph");
    expectText(std::to_string(P::listItemGap) + " pixels between two list items");
    expectText(std::to_string(P::h2Above) + " pixels above it (level 2) or " + std::to_string(P::h3Above) + " pixels (level 3), and "
               + std::to_string(P::h2Below) + " or " + std::to_string(P::h3Below) + " pixels below it");
    expectText("**Column.** " + std::to_string(P::measure) + " pixels wide, centered, with at least " + std::to_string(P::pageMargin) + " pixels of margin");

    // The lines of the headings are 1.25 to 1.6 times the size, as the page says.
    for (auto [size, line] : { std::pair{ P::h1Size, P::h1Line }, { P::h2Size, P::h2Line }, { P::h3Size, P::h3Line } }) {
        EXPECT_GE(line / (double) size, 1.25);
        EXPECT_LE(line / (double) size, 1.6);
    }
    EXPECT_DOUBLE_EQ(P::bodyLine / (double) P::bodySize, 1.75);
}

// Each font file is in Git LFS, and each family has a license file.
TEST(Fonts, TP_FONT_06_EachFontFileIsInGitLfsAndEachLicenseIsInTheResources)
{
    std::vector<fs::path> fonts;
    for (const auto &entry : fs::directory_iterator(root / "resources")) {
        const std::string extension = entry.path().extension().string();
        if (extension == ".ttf" || extension == ".otf") fonts.push_back(entry.path());
    }
    ASSERT_FALSE(fonts.empty());

    int status = 0;
    runCommand("git --version 2>&1", &status);
    const bool haveGit = status == 0;

    std::set<std::string> families;
    for (const fs::path &font : fonts) {
        // The family is the part of the name before the dash: Roboto-Bold.ttf and Roboto.ttf are Roboto.
        std::string family = font.stem().string();
        family = family.substr(0, family.find('-'));
        families.insert(family);

        if (haveGit) {
            const std::string out = runCommand("git -C \"" + root.string() + "\" check-attr filter -- \"" + font.string() + "\" 2>&1");
            EXPECT_NE(out.find("filter: lfs"), std::string::npos) << font.filename() << ": " << out;
        }
    }
    for (const std::string &family : families) {
        const fs::path license = root / "resources" / ("LICENSE-" + family + ".txt");
        EXPECT_TRUE(fs::exists(license)) << "no license file for " << family;
        EXPECT_GT(fs::exists(license) ? fs::file_size(license) : 0, 100u) << license.filename();
    }

    // The fonts are in a resource file, so the windows look the same on each Mac.
    const std::string qrc = readFile(root / "resources/other.qrc") + readFile(root / "resources/qml.qrc");
    for (const fs::path &font : fonts) {
        EXPECT_NE(qrc.find(font.filename().string()), std::string::npos) << font.filename() << " is in no resource file";
    }
}

// A release commit has no test hook. The hook is temporary, and a script puts it in the file and takes it out.
TEST(Release, TP_REL_05_TheTestHookIsNotInTheSource)
{
    int checked = 0;
    for (const auto &entry : fs::recursive_directory_iterator(root / "src")) {
        const std::string extension = entry.path().extension().string();
        if (extension != ".cpp" && extension != ".h" && extension != ".mm" && extension != ".qml") continue;
        const std::string text = readFile(entry.path());
        EXPECT_EQ(text.find("TEMPORARY TEST HOOK"), std::string::npos) << entry.path();
        EXPECT_EQ(text.find("INFORMANT_GRAB"), std::string::npos) << entry.path();
        ++checked;
    }
    EXPECT_GT(checked, 50);
    EXPECT_TRUE(fs::exists(root / "src/context/guicontext.cpp"));
}

// The program runs on macOS only. The code of the app has no condition for another system, no choice of
// audio backend, and no optional library. The files of other authors in `src` keep their own conditions.
TEST(Release, TP_REL_08_TheSourceHasNoCodeForOtherSystems)
{
    const std::vector<std::string> vendored = { "atomicops.h", "readerwriterqueue.h" };
    const std::vector<std::string> banned = { "_WIN32", "__ANDROID__", "__linux__", "__EMSCRIPTEN__",
        "ENABLE_TORCH", "WITHOUT_SYNTH", "AUDIO_USE_", "Backend::", "SYSTEM_WINDOWS", "SYSTEM_LINUX" };
    int checked = 0;
    for (const auto &entry : fs::recursive_directory_iterator(root / "src")) {
        const std::string extension = entry.path().extension().string();
        if (extension != ".cpp" && extension != ".h" && extension != ".mm" && extension != ".qml") continue;
        if (std::find(vendored.begin(), vendored.end(), entry.path().filename().string()) != vendored.end()) continue;
        const std::string text = readFile(entry.path());
        for (const std::string &word : banned) {
            EXPECT_EQ(text.find(word), std::string::npos) << entry.path() << " has " << word;
        }
        ++checked;
    }
    EXPECT_GT(checked, 50);

    // The files of the other systems are gone, and the build file stops on any other system.
    for (const char *gone : { "docker", "src/windows_debug.cpp", "src/android_redirect_log.cpp", "cmake_support", "build.sh" }) {
        EXPECT_FALSE(fs::exists(root / gone)) << gone;
    }
    const std::string cmake = readFile(root / "CMakeLists.txt");
    EXPECT_NE(cmake.find("InFormant builds on macOS only"), std::string::npos);
    EXPECT_EQ(cmake.find("ANDROID"), std::string::npos);
    EXPECT_EQ(cmake.find("MSVC"), std::string::npos);
}

// The build script downloads only over https, checks a pinned SHA-256 checksum of each download, and pins the
// versions of its Python tools. The test script uses the same Qt version as the build script.
TEST(Release, TP_REL_09_TheBuildScriptPinsAndChecksItsDownloads)
{
    const std::string build = readFile(root / "build-macos.sh");
    EXPECT_EQ(build.find("http://"), std::string::npos);

    int fetches = 0;
    for (const std::string &line : readLines(root / "build-macos.sh")) {
        if (line.find("    fetch ") == 0) {
            ++fetches;
            EXPECT_NE(line.find("_SHA256\""), std::string::npos) << line;
        }
    }
    EXPECT_EQ(fetches, 3);

    for (const char *name : { "FFTW_SHA256=", "PORTAUDIO_SHA256=", "EIGEN_SHA256=" }) {
        const size_t at = build.find(name);
        ASSERT_NE(at, std::string::npos) << name;
        const std::string sum = build.substr(at + std::string(name).size(), 64);
        EXPECT_EQ(sum.find_first_not_of("0123456789abcdef"), std::string::npos) << name;
        EXPECT_EQ(build[at + std::string(name).size() + 64], '\n') << name;
    }

    for (const char *pin : { "AQT_VERSION=", "CMAKE_VERSION=", "NINJA_VERSION=" }) {
        EXPECT_NE(build.find(pin), std::string::npos) << pin;
    }

    // Both scripts name the same Qt version.
    const auto qtVersion = [](const std::string &text) {
        const std::string key = "QT_VERSION=${QT_VERSION:-";
        const size_t at = text.find(key);
        return at == std::string::npos ? std::string() : text.substr(at + key.size(), text.find('}', at) - at - key.size());
    };
    const std::string tests = readFile(root / "run-tests.sh");
    EXPECT_FALSE(qtVersion(build).empty());
    EXPECT_EQ(qtVersion(build), qtVersion(tests));
}

// The signing rules. The default is an ad hoc signature. A Developer ID identity adds the hardened runtime, the
// entitlements for the microphone and the Qt Quick compiler, and the secure timestamp. The microphone text is in
// the Info.plist, because macOS ends the program without it when the program asks for the microphone.
TEST(Release, TP_REL_11_TheSigningRulesAndTheMicrophoneEntitlementAreInPlace)
{
    const std::string build = readFile(root / "build-macos.sh");
    EXPECT_NE(build.find("SIGN_IDENTITY=${SIGN_IDENTITY:--}"), std::string::npos);
    EXPECT_NE(build.find("--options runtime"), std::string::npos);
    EXPECT_NE(build.find("--entitlements"), std::string::npos);
    EXPECT_NE(build.find("--timestamp"), std::string::npos);
    EXPECT_NE(build.find("notarytool submit"), std::string::npos);
    EXPECT_NE(build.find("stapler staple"), std::string::npos);

    const std::string entitlements = readFile(root / "dist-res/InFormant.entitlements");
    EXPECT_NE(entitlements.find("com.apple.security.device.audio-input"), std::string::npos);
    EXPECT_NE(entitlements.find("com.apple.security.cs.allow-jit"), std::string::npos);

    // Qt reports a denied microphone when the static permission plugin is not in the program.
    const std::string cmake = readFile(root / "CMakeLists.txt");
    EXPECT_NE(cmake.find("qt6_import_plugins(in-formant INCLUDE Qt6::QDarwinMicrophonePermissionPlugin)"), std::string::npos);

    const std::string plist = readFile(root / "dist-res/Info.plist");
    const size_t at = plist.find("NSMicrophoneUsageDescription");
    ASSERT_NE(at, std::string::npos);
    EXPECT_NE(plist.find("<string>", at), std::string::npos);
}

namespace {

struct Block {
    std::string file;
    int line;
    std::string text;
};

// Each item that wraps its text. The block is the text of the item, from its opening brace to the closing brace.
std::vector<Block> wrappingItems(const fs::path &file)
{
    std::vector<Block> blocks;
    const std::string text = readFile(file);
    size_t position = 0;
    while ((position = text.find("wrapMode:", position)) != std::string::npos) {
        size_t open = position;
        int depth = 0;
        while (open > 0) {
            --open;
            if (text[open] == '}') ++depth;
            if (text[open] == '{') {
                if (depth == 0) break;
                --depth;
            }
        }
        size_t close = position;
        depth = 0;
        for (; close < text.size(); ++close) {
            if (text[close] == '{') ++depth;
            if (text[close] == '}') {
                if (depth == 0) break;
                --depth;
            }
        }
        const int line = 1 + (int) std::count(text.begin(), text.begin() + (long) position, '\n');
        blocks.push_back({ file.filename().string(), line, text.substr(open + 1, close - open - 1) });
        position += 9;
    }
    return blocks;
}

}

// A paragraph in a window has at most 560 pixels (the text measure). The help pages are an exception.
// They have their own column of 640 pixels, and a "?" box has at most 380 pixels.
TEST(Layout, TP_UI_12_NoParagraphInAWindowIsWiderThan560Pixels)
{
    const std::regex maximum(R"(maximumWidth:\s*([0-9]+|ui\.measure))");
    const std::regex fixed(R"((?:^|\n)\s*(?:Layout\.preferredWidth|width):\s*([0-9]+)\s*\n)");

    int checked = 0;
    for (const auto &entry : fs::recursive_directory_iterator(root / "src/qml")) {
        if (entry.path().extension() != ".qml") continue;
        const std::string name = entry.path().filename().string();
        // A "?" box caps its own width at 380 pixels, and a help page uses the prose scale (640 pixels).
        if (name == "HelpButton.qml" || name == "HelpWindow.qml") continue;

        for (const Block &block : wrappingItems(entry.path())) {
            ++checked;
            std::smatch m;
            const bool fills = block.text.find("Layout.fillWidth: true") != std::string::npos;
            const std::string where = block.file + ":" + std::to_string(block.line);

            if (std::regex_search(block.text, m, maximum)) {
                const int limit = m[1] == "ui.measure" ? Gui::UiScale::textMeasure : std::stoi(m[1]);
                EXPECT_LE(limit, Gui::UiScale::textMeasure) << where;
            }
            else if (!fills && std::regex_search(block.text, m, fixed)) {
                EXPECT_LE(std::stoi(m[1]), Gui::UiScale::textMeasure) << where;
            }
            else if (name == "Hint.qml") {
                ADD_FAILURE() << where << ": the Hint must have a maximum width";
            }
            else {
                ADD_FAILURE() << where << ": a paragraph needs a maximum width of 560 pixels or less";
            }
        }
    }
    EXPECT_GE(checked, 8);
    EXPECT_EQ(Gui::UiScale::textMeasure, 560);
}

// The FFT table on the page of the settings follows the formula for a view up to 5500 Hz.
TEST(Docs, TP_GDE_02_TheFftTableMatchesTheFormulaForAViewUpTo5500Hz)
{
    const std::string text = readFile(root / "docs/choosing-settings.md");
    const double highest = 5500.0;
    // The page gives the formula in words. Check that both parts are on the page.
    EXPECT_NE(text.find("The window time is the FFT size divided by twice the highest frequency of the view."), std::string::npos);
    EXPECT_NE(text.find("the highest frequency of the view, times two, divided by the FFT size"), std::string::npos);
    EXPECT_NE(text.find("a view up to 5500 Hz"), std::string::npos);

    const std::regex row(R"(- \*\*(\d+)\.\*\* Window (\d+) ms\. Lines ([0-9.]+) Hz apart\.)");
    std::vector<int> sizes;
    for (auto it = std::sregex_iterator(text.begin(), text.end(), row); it != std::sregex_iterator(); ++it) {
        const int size = std::stoi((*it)[1]);
        sizes.push_back(size);
        const double windowMs = 1000.0 * size / (2.0 * highest);
        const double lineHz = 2.0 * highest / size;
        EXPECT_EQ(std::stoi((*it)[2]), (int) std::lround(windowMs)) << "FFT size " << size;
        EXPECT_NEAR(std::stod((*it)[3]), lineHz, 0.05) << "FFT size " << size;
    }
    EXPECT_EQ(sizes, (std::vector<int>{ 512, 1024, 2048, 4096 }));
}
