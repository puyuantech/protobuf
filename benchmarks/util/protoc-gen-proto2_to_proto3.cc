#include "google/protobuf/compiler/code_generator.h"
#include "google/protobuf/io/zero_copy_stream.h"
#include "google/protobuf/io/printer.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"
#include "schema_proto2_to_proto3_util.h"

#include "google/protobuf/compiler/plugin.h"

using linker::google::protobuf::FileDescriptorProto;
using linker::google::protobuf::FileDescriptor;
using linker::google::protobuf::DescriptorPool;
using linker::google::protobuf::io::Printer;
using linker::google::protobuf::util::SchemaGroupStripper;
using linker::google::protobuf::util::EnumScrubber;
using linker::google::protobuf::util::ExtensionStripper;
using linker::google::protobuf::util::FieldScrubber;

namespace linker {
namespace google {
namespace protobuf {
namespace compiler {

namespace {

string StripProto(string filename) {
  return filename.substr(0, filename.rfind(".proto"));
}

DescriptorPool* GetPool() {
  static DescriptorPool *pool = new DescriptorPool();
  return pool;
}

}  // namespace

class Proto2ToProto3Generator final : public CodeGenerator {
 public:
  bool GenerateAll(const std::vector<const FileDescriptor*>& files,
                           const string& parameter,
                           GeneratorContext* context,
                           string* error) const {
    for (int i = 0; i < files.size(); i++) {
      for (auto file : files) {
        if (CanGenerate(file)) {
          Generate(file, parameter, context, error);
          break;
        }
      }
    }

    return true;
  }

  bool Generate(const FileDescriptor* file,
                        const string& parameter,
                        GeneratorContext* context,
                        string* error) const {
    FileDescriptorProto new_file;
    file->CopyTo(&new_file);
    SchemaGroupStripper::StripFile(file, &new_file);

    EnumScrubber enum_scrubber;
    enum_scrubber.ScrubFile(&new_file);
    ExtensionStripper::StripFile(&new_file);
    FieldScrubber::ScrubFile(&new_file);
    new_file.set_syntax("proto3");

    string filename = file->name();
    string basename = StripProto(filename);

    std::vector<std::pair<string,string>> option_pairs;
    ParseGeneratorParameter(parameter, &option_pairs);

    std::unique_ptr<linker::google::protobuf::io::ZeroCopyOutputStream> output(
        context->Open(basename + ".proto"));
    string content = GetPool()->BuildFile(new_file)->DebugString();
    Printer printer(output.get(), '$');
    printer.WriteRaw(content.c_str(), content.size());

    return true;
  }
 private:
  bool CanGenerate(const FileDescriptor* file) const {
    if (GetPool()->FindFileByName(file->name()) != nullptr) {
      return false;
    }
    for (int j = 0; j < file->dependency_count(); j++) {
      if (GetPool()->FindFileByName(file->dependency(j)->name()) == nullptr) {
        return false;
      }
    }
    for (int j = 0; j < file->public_dependency_count(); j++) {
      if (GetPool()->FindFileByName(
          file->public_dependency(j)->name()) == nullptr) {
        return false;
      }
    }
    for (int j = 0; j < file->weak_dependency_count(); j++) {
      if (GetPool()->FindFileByName(
          file->weak_dependency(j)->name()) == nullptr) {
        return false;
      }
    }
    return true;
  }
};

}  // namespace compiler
}  // namespace protobuf
}  // namespace google
}  // namespace linker

int main(int argc, char* argv[]) {
  linker::google::protobuf::compiler::Proto2ToProto3Generator generator;
  return linker::google::protobuf::compiler::PluginMain(argc, argv, &generator);
}
