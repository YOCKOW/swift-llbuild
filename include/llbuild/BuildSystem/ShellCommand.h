//===- ShellCommand.h -------------------------------------------*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2019 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See http://swift.org/LICENSE.txt for license information
// See http://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//

#ifndef LLBUILD_BUILDSYSTEM_SHELLCOMMAND_H
#define LLBUILD_BUILDSYSTEM_SHELLCOMMAND_H

#include "llbuild/BuildSystem/BuildSystemHandlers.h"
#include "llbuild/BuildSystem/ExternalCommand.h"

#include "llbuild/Basic/ShellUtility.h"

#include "llvm/ADT/Optional.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/MemoryBuffer.h"

#include <atomic>
#include <string>
#include <vector>

namespace llbuild {
  namespace basic {
    class QueueJobContext;
  }
  namespace core {
    class Task;
  }
  
namespace buildsystem {

class BuildNode;
class BuildSystem;
class ExternalCommandHandler;

class ShellCommand : public ExternalCommand {
  /// The dependencies style to expect (in the `depsPath`).
  enum class DepsStyle {
    /// No discovered dependencies are in use.
    Unused = 0,
      
    /// "Makefile" style dependencies in the form typically generated by C
    /// compilers, wherein the dependencies of the first target are treated as
    /// dependencies of the command.
    Makefile,

    /// Darwin's DependencyInfo format.
    DependencyInfo,
  };
  
  /// The command line arguments.
  std::vector<StringRef> args;

  /// Arbitrary string used to contribute to the task signature.
  std::string signatureData;

  /// The environment to use. If empty, the environment will be inherited.
  SmallVector<std::pair<StringRef, StringRef>, 1> env;
  
  /// The path to the dependency output file, if used.
  SmallVector<std::string, 1> depsPaths{};

  /// The style of dependencies used.
  DepsStyle depsStyle = DepsStyle::Unused;

  /// Whether to inherit the base environment.
  bool inheritEnv = true;

  /// Whether it is safe to interrupt (SIGINT) the tool during cancellation.
  bool canSafelyInterrupt = true;

  /// Working directory in which to spawn the external command
  std::string workingDirectory;

  /// Whether the control pipe is enabled for this command
  bool controlEnabled = true;

  /// The cached signature, once computed -- 0 is used as a sentinel value.
  mutable std::atomic<basic::CommandSignature> cachedSignature{ };

  /// The handler to use for this command, if present.
  ShellCommandHandler* handler;

  /// The handler state, if used.
  std::unique_ptr<HandlerState> handlerState;

  virtual void start(BuildSystemCommandInterface& bsci,
                     core::Task* task) override;
  
  virtual basic::CommandSignature getSignature() const override;

  bool processDiscoveredDependencies(BuildSystemCommandInterface& bsci,
                                     core::Task* task,
                                     basic::QueueJobContext* context);
  
  bool processMakefileDiscoveredDependencies(BuildSystemCommandInterface& bsci,
                                             core::Task* task,
                                             basic::QueueJobContext* context,
                                             StringRef depsPath,
                                             llvm::MemoryBuffer* input);

  bool
  processDependencyInfoDiscoveredDependencies(BuildSystemCommandInterface& bsci,
                                              core::Task* task,
                                              basic::QueueJobContext* context,
                                              StringRef depsPath,
                                              llvm::MemoryBuffer* input);

public:
  using ExternalCommand::ExternalCommand;
  ShellCommand(StringRef name, bool controlEnabled) : ExternalCommand(name),
    controlEnabled(controlEnabled) { }

  virtual const std::vector<StringRef>& getArgs() const { return args; }

  virtual const SmallVector<std::pair<StringRef, StringRef>, 1>&
  getEnv() const { return env; }

  bool getInheritEnv() const { return inheritEnv; }
  
  virtual void getShortDescription(SmallVectorImpl<char> &result) const override {
    llvm::raw_svector_ostream(result) << getDescription();
  }

  virtual void getVerboseDescription(SmallVectorImpl<char> &result) const override {
    llvm::raw_svector_ostream os(result);
    bool first = true;
    for (const auto& arg: args) {
      if (!first) os << " ";
      first = false;
      basic::appendShellEscapedString(os, arg);
    }
  }
  
  virtual bool configureAttribute(const ConfigureContext& ctx, StringRef name,
                                  StringRef value) override;
  
  virtual bool configureAttribute(const ConfigureContext& ctx, StringRef name,
                                  ArrayRef<StringRef> values) override;

  virtual bool configureAttribute(
      const ConfigureContext& ctx, StringRef name,
      ArrayRef<std::pair<StringRef, StringRef>> values) override;

  virtual void executeExternalCommand(
      BuildSystemCommandInterface& bsci,
      core::Task* task,
      basic::QueueJobContext* context,
      llvm::Optional<basic::ProcessCompletionFn> completionFn) override;
};

}
}

#endif
