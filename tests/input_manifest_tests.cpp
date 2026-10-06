#include "test.hpp"
#include "../src/project/input_manifest.hpp"
#include "../src/project/run.hpp"
#include "../src/project/evaluation.hpp"
#include <fstream>
#include <iterator>
using namespace trafficsim;

TEST(provenance, sha256_known_answers_and_multiblock_padding) {
    CHECK(inputSha256("")=="e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    CHECK(inputSha256("abc")=="ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    CHECK(inputSha256("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq")==
          "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1");
    CHECK(inputSha256(std::string(1000000,'a'))=="cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0");
}
TEST(provenance, stable_logical_order_repeat_count_and_changed_read_guard) {
    InputManifest m;m.record("z","abc");m.record("a","");m.record("z","abc");
    const auto j=m.json();CHECK(j["algorithm"]=="SHA-256");CHECK(j["files"].size()==2);
    CHECK(j["files"][0]["logicalPath"]=="a");CHECK(j["files"][1]["reads"]==2);
    CHECK(j["files"][1]["bytes"]==3);
    CHECK(j["files"][1]["sha256"]=="ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    // Establish the differing digest before forcing the changed-file guard.
    CHECK(inputSha256("abcd")!=inputSha256("abc"));
    test::throws([&]{m.record("z","abcd");},"Input changed");
    test::throws([&]{m.validate();},"Input changed");test::throws([&]{m.json();});
}
TEST(provenance, exact_project_bytes_and_catalog_reads_preserve_compilation) {
    InputManifest m;
    const auto file=test::root()/"data/projects/four-leg-signalised.traffic.json";
    std::ifstream stream(file,std::ios::binary);CHECK(stream.is_open());
    const std::string bytes{std::istreambuf_iterator<char>(stream),std::istreambuf_iterator<char>()};
    CHECK(!stream.bad() && !bytes.empty());
    const auto document=parseDocument(readInputJson(file,"project",&m));
    const auto measured=compileDocument(document,test::root()/"data",&m);
    const auto original=compileDocument(document,test::root()/"data");
    CHECK(static_cast<const ScenarioDefinition&>(measured.scenario)==static_cast<const ScenarioDefinition&>(original.scenario));
    CHECK(measured.network==original.network);
    CHECK(measured.scenario.segments.size()==original.scenario.segments.size());
    for(std::size_t i=0;i<measured.scenario.segments.size();++i) {
        const auto& a=measured.scenario.segments[i];const auto& b=original.scenario.segments[i];
        CHECK(a.id==b.id && a.length==b.length && a.next==b.next);
    }
    CHECK(measured.scenario.signalHeads.size()==original.scenario.signalHeads.size());
    for(std::size_t i=0;i<measured.scenario.signalHeads.size();++i) {
        const auto& a=measured.scenario.signalHeads[i];const auto& b=original.scenario.signalHeads[i];
        CHECK(a.id==b.id && a.segmentId==b.segmentId && a.position==b.position && a.programId==b.programId);
    }
    const auto spec=evaluationSpec(document,measured,test::root()/"data",&m);
    CHECK(spec.queue.maxGap>0);const auto j=m.json();bool project=false,queue=false,types=false,behaviour=false;
    for(const auto& e:j["files"]) {
        const auto name=e["logicalPath"].get<std::string>();
        if(name=="project") {
            project=true;CHECK(e["sha256"]==inputSha256(bytes));CHECK(e["bytes"]==bytes.size());
        }
        if(name=="evaluation/queue-counter.json")queue=true;
        types|=name.starts_with("catalog/vehicle-types/");behaviour|=name.starts_with("catalog/driver-behaviour/");
    }
    CHECK(project && queue && types && behaviour);
}
TEST(provenance, equivalent_json_retains_distinct_lf_and_crlf_bytes) {
    const std::string lf="{\n  \"value\": 42\n}\n",crlf="{\r\n  \"value\": 42\r\n}\r\n";
    CHECK(Json::parse(lf)==Json::parse(crlf));CHECK(lf.size()==18 && crlf.size()==21);
    const auto directory=std::filesystem::temp_directory_path()/"trafficsim-input-line-endings";
    std::filesystem::create_directories(directory);
    struct Cleanup {
        std::filesystem::path path;
        ~Cleanup() {std::error_code error;std::filesystem::remove_all(path,error);}
    } cleanup{directory};
    const auto file=directory/"input.json";
    const auto write=[&](const std::string& bytes) {
        std::ofstream stream(file,std::ios::binary);CHECK(stream.is_open());
        stream.write(bytes.data(),static_cast<std::streamsize>(bytes.size()));
        stream.close();CHECK(!stream.fail());
    };
    write(lf);InputManifest a;const auto parsed=readInputJson(file,"project",&a);
    const auto first=a.json()["files"][0];CHECK(first["bytes"]==18);
    // Independent hashlib answers: checkout line endings must never be normalized.
    CHECK(first["sha256"]=="4c7433d86dcfac84280ccbf1704e4fb540e511085a4fd0bd45f28b85e3400163");
    write(crlf);InputManifest b;CHECK(readInputJson(file,"project",&b)==parsed);
    const auto second=b.json()["files"][0];CHECK(second["bytes"]==21);
    CHECK(second["sha256"]=="68641a2550477f0ea086f3bad36f9ec5cb700ffaa6e1bccc6ec1198960967aff");
    CHECK(first["sha256"]!=second["sha256"]);
    test::throws([&]{readInputJson(file,"project",&a);},"Input changed");
    test::throws([&]{a.validate();},"Input changed");
}
TEST(provenance, optional_fallbacks_are_reported_and_deduplicated) {
    InputManifest m;m.fallback("catalog/priority-rules");m.fallback("catalog/priority-rules");
    CHECK(m.json()["fallbacks"]==Json::array({"catalog/priority-rules"}));
}

TEST(provenance, binary_bytes_at_sha256_padding_boundaries) {
    const std::vector<std::pair<std::size_t,std::string>> answers{
        {1,"4a64a107f0cb32536e5bce6c98c393db21cca7f4ea187ba8c4dca8b51d4ea80a"},
        {55,"59aaae80b8e7958f5757b4ac9274f1fd57ec0dc7aa8349319102316781b92006"},
        {56,"dca902d31487ffab357ce36cc5abc6947fbb69127ec04c468ba5687268c4bf7c"},
        {63,"b8e655e9e7ad413b96f0a371eb1d6db71dcca27f4eee35892517cbe4bfaa6f9a"},
        {64,"e5146be62accc56709594cb45c651b361df94f622cbb09b91ea3ca7a2060bd53"},
        {65,"4b6c6774f0cbcd776a90e187b9494e04ca880c0afe3ea1c5cad51c8bbcb41f74"},
        {119,"3244509f8746f6fb511ca8a72a292a4d7d12e456cce283c03f169dbbcb0d512e"},
        {120,"c5236e50172da3b69c903dcd3b3bb8ce855182f79364d1b59ed36367aa25e468"},
        {127,"b4f5398bf618d741f2ab0f6e2fcde06f4949c20d476b5f6e1d4bf8618bc8973a"},
        {128,"ed045c7c319e0c9286e1dea21c80bfd4a08eca43e02e13ee8a7a2c8b4b2baec9"},
        {129,"d6f2fca0407b3f98c9ce29ae92a6cdf489506450fdbb29172f6b18987bc685db"}};
    for(const auto& [n,expected]:answers) {
        std::string bytes(n,'\0');for(std::size_t i=0;i<n;++i)bytes[i]=static_cast<char>((i*131+17)%256);
        CHECK(inputSha256(bytes)==expected);
    }
}
TEST(provenance, owned_catalogs_skip_external_reads_and_report_priority_fallback) {
    AuthoringDefinition owned;owned.externalVehicleTypes=false;owned.externalBehaviours=false;owned.externalCompositions=false;
    owned.vehicleTypes={{"car",4,2,{10,10},2,3,6,"driver"}};
    owned.behaviours={{"driver",2,2,3,1.5,.2}};
    InputManifest m;const auto absent=test::root()/"data/no-such-catalog-directory";
    CHECK(!std::filesystem::exists(absent));
    const auto resolved=resolveCatalogs(owned,absent,&m);
    CHECK(resolved.vehicleTypes==owned.vehicleTypes && resolved.behaviours==owned.behaviours);
    CHECK(m.json()["files"].empty());
    CHECK(m.json()["fallbacks"]==Json::array({"catalog/priority-rules"}));
}
