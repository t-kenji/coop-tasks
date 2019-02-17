# Make rules for some project.

ifeq ($(PLATFORM),native)
  EXTRA_CPPFLAGS +=
  CROSS_COMPILE :=
  CPPCHECK_PLAT := native
else ifeq ($(PLATFORM),armhf)
  EXTRA_CPPFLAGS +=
  CROSS_COMPILE := arm-linux-gnueabihf-
  CPPCHECK_PLAT := unix32
else
  $(error "$(PLATFORM) Unknown platform")
endif

ifneq ($(BUILD_TYPE),release)
  EXTRA_CFLAGS += -fprofile-arcs -ftest-coverage
  EXTRA_LDLIBS += -lgcov
endif

INCS := -I. $(EXTRA_INCS)

OPT_WARN := -Wall -Wextra -Wshadow -Wcast-align
OPT_WARN += $(if $(filter $(WARN_AS_ERROR),1),-Werror)
OPT_WARN += -Wno-clobbered # workarround for pthread_cleanup_push() bug
OPT_WARN += -Wno-missing-field-initializers
OPT_OPTIM := $(if $(filter $(BUILD_TYPE),release),-O2,-Og)
OPT_OPTIM += $(if $(or $(LIBRARY), $(filter-out $(BUILD_TYPE),release)),-fPIC)
OPT_DEBUG := $(if $(filter-out $(BUILD_TYPE),release),-g)
ifneq ($(and $(filter-out $(BUILD_TYPE),release), $(filter $(ENABLE_SANITIZER),1)),)
  OPT_DEBUG += -fsanitize=address -fsanitize=leak -fno-omit-frame-pointer
endif
OPT_DEP := -MMD -MP

OPTS := $(OPT_WARN) $(OPT_OPTIM) $(OPT_DEBUG) $(OPT_DEP)
DEFS := -DMODULE_VERSION=\"$(VERSION)\"
DEFS += $(if $(NODEBUG),-DNODEBUG=$(NODEBUG))
DEFS += $(if $(INTERNAL_TESTABLE),-DINTERNAL_TESTABLE=$(INTERNAL_TESTABLE))

CPPFLAGS := $(DEFS) $(EXTRA_CPPFLAGS)
CFLAGS := $(if $(CSTANDARD),-std=$(CSTANDARD)) $(OPTS) -fdiagnostics-color $(INCS) $(EXTRA_CFLAGS)
CXXFLAGS := $(if $(CXXSTANDARD),-std=$(CXXSTANDARD)) $(OPTS) -fdiagnostics-color $(INCS) $(EXTRA_CXXFLAGS)
LDFLAGS := $(if $(or $(ENABLE_STATIC),$(ENABLE_SHARED)),-L$(ROOTDIR)/src) $(EXTRA_LDFLAGS)
CLDLIBS := $(if $(or $(ENABLE_STATIC),$(ENABLE_SHARED)),-l$(NAME))
ifneq ($(and $(filter-out $(BUILD_TYPE),release), $(filter $(ENABLE_SANITIZER),1)),)
  CLDLIBS += -lasan -lstdc++
endif
CLDLIBS += $(EXTRA_LDLIBS)
CXXLDLIBS := $(if $(or $(ENABLE_STATIC),$(ENABLE_SHARED)),-l$(NAME))
ifneq ($(and $(filter-out $(BUILD_TYPE),release), $(filter $(ENABLE_SANITIZER),1)),)
  CXXLDLIBS += -lasan
endif
CXXLDLIBS += $(EXTRA_LDLIBS)
ARFLAGS := rcs

Q1     = $(V:1=)
QCC    = $(Q1:0=@echo "  CC    $@";)
QCXX   = $(Q1:0=@echo "  CXX   $@";)
QLD    = $(Q1:0=@echo "  LD    $@";)
QAR    = $(Q1:0=@echo "  AR    $@";)
QLINK  = $(Q1:0=@echo "  LINK  $@";)
QSTRIP = $(Q1:0=@echo "  STRIP $@";)
QCLEAN = $(Q1:0=@echo "  CLEAN $(EXECUTABLE)$(LIBRARY)$(TEST)";)

ifneq ($(DISABLE_CCACHE),1)
  CCACHE := $(shell which ccache)
endif
CC := $(CCACHE) $(CROSS_COMPILE)gcc
CXX := $(CCACHE) $(CROSS_COMPILE)g++
LD := $(CROSS_COMPILE)ld
AR := $(CROSS_COMPILE)ar
STRIP := $(CROSS_COMPILE)strip

ifeq ($(ENABLE_STATIC),1)
  STATIC_LIBRARY := $(LIBRARY).a.$(VERSION)
endif
ifeq ($(ENABLE_SHARED),1)
  SHARED_LIBRARY := $(LIBRARY).so.$(VERSION)
endif
DEPS := $(OBJS:.o=.d)
GCDAS := $(OBJS:.o=.gcda)
GCNOS := $(OBJS:.o=.gcno)

%.o: %.c
	$(QCC)$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

%.o: %.cpp
	$(QCXX)$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

.PHONY: all \
        $(EXECUTABLE) \
        $(LIBRARY) $(STATIC_LIBRARY) $(SHARED_LIBRARY) \
        $(TEST) \
        clean

all: $(EXECUTABLE) $(LIBRARY) $(TEST)

$(EXECUTABLE): $(OBJS)
	$(QLINK)$(CC) $(LDFLAGS) -o $@ $^ $(CLDLIBS)
ifeq ($(BUILD_TYPE),release)
	$(QSTRIP)$(STRIP) -s $@
endif

$(LIBRARY): $(STATIC_LIBRARY) $(SHARED_LIBRARY)

$(STATIC_LIBRARY): $(OBJS)
	$(QAR)$(AR) $(ARFLAGS) $@ $^
	@ln -fs $@ $(@:.$(VERSION)=)

$(SHARED_LIBRARY): $(OBJS)
	$(QLD)$(LD) -shared -fPIC -o $@ $^
	@ln -fs $@ $(@:.$(VERSION)=)

$(TEST): $(OBJS)
	$(QLINK)$(CXX) $(LDFLAGS) -o $@ $^ $(CXXLDLIBS)

cppcheck:
	@cppcheck --quiet --enable=all --check-config --verbose \
	          $(if $(CPPCHECK_PLAT),--platform=$(CPPCHECK_PLAT)) \
	          $(if $(CSTANDARD),--std=$(CSTANDARD)) \
	          --suppress=unusedFunction --suppress=missingInclude \
	          $(CPPFLAGS) \
	          $(INCS) \
	          .

clean:
	$(QCLEAN)rm -rf $(if $(LIBRARY),$(LIBRARY).a* $(LIBRARY).so*) $(EXECUTABLE) $(TEST) $(OBJS) $(DEPS) $(GCDAS) $(GCNOS)

-include $(DEPS)
