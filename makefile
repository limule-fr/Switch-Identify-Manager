
CXX = g++

CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic -Iinclude

TARGET = build/sim-sandbox
INTEGRITY_TEST_TARGET = build/integrity-checker-tests
BACKUP_TEST_TARGET = build/backup-manager-tests
SYSTEM_INSPECTOR_TEST_TARGET = build/system-inspector-tests
REPORT_GENERATOR_TEST_TARGET = build/report-generator-tests
SETTINGS_TEST_TARGET = build/settings-tests
LOGGER_TEST_TARGET = build/logger-tests
SHA256_TEST_TARGET = build/sha256-tests
SWITCH_ELF = build/switch/sim.elf
SWITCH_NRO = build/switch/sim.nro

SWITCH_CXX = $(DEVKITPRO)/devkitA64/bin/aarch64-none-elf-g++
SWITCH_FLAGS = -std=gnu++17 -O2 -Wall -Wextra -Wpedantic \
               -march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft \
               -fPIE -ffunction-sections -fdata-sections \
               -fno-rtti -fno-exceptions -D__SWITCH__ \
               -Iinclude -I$(DEVKITPRO)/libnx/include

SOURCES = source/main.cpp \
          source/system/prodinfo.cpp \
          source/system/integrity_checker.cpp \
          source/system/backup_manager.cpp \
          source/system/system_inspector.cpp \
          source/system/report_generator.cpp \
          source/system/localization.cpp \
          source/system/settings_manager.cpp \
          source/system/logger.cpp \
          source/system/sha256.cpp

INTEGRITY_TEST_SOURCES = tests/integrity_checker_test.cpp \
                         source/system/prodinfo.cpp \
                         source/system/integrity_checker.cpp \
                         source/system/localization.cpp

BACKUP_TEST_SOURCES = tests/backup_manager_test.cpp \
                      source/system/backup_manager.cpp \
                      source/system/sha256.cpp

SYSTEM_INSPECTOR_TEST_SOURCES = tests/system_inspector_test.cpp \
                                source/system/system_inspector.cpp \
                                source/system/localization.cpp

REPORT_GENERATOR_TEST_SOURCES = tests/report_generator_test.cpp \
                                source/system/report_generator.cpp \
                                source/system/localization.cpp

SETTINGS_TEST_SOURCES = tests/settings_manager_test.cpp \
                        source/system/settings_manager.cpp \
                        source/system/localization.cpp

LOGGER_TEST_SOURCES = tests/logger_test.cpp \
                      source/system/logger.cpp

SHA256_TEST_SOURCES = tests/sha256_test.cpp \
                      source/system/sha256.cpp

all: $(TARGET)

$(TARGET): $(SOURCES) include/prodinfo.hpp include/integrity_checker.hpp include/backup_manager.hpp include/system_inspector.hpp include/report_generator.hpp include/localization.hpp include/settings_manager.hpp include/logger.hpp include/sha256.hpp
	mkdir -p build
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(TARGET)

$(INTEGRITY_TEST_TARGET): $(INTEGRITY_TEST_SOURCES) include/prodinfo.hpp include/integrity_checker.hpp include/localization.hpp
	mkdir -p build
	$(CXX) $(CXXFLAGS) $(INTEGRITY_TEST_SOURCES) -o $(INTEGRITY_TEST_TARGET)

$(BACKUP_TEST_TARGET): $(BACKUP_TEST_SOURCES) include/backup_manager.hpp include/sha256.hpp
	mkdir -p build
	$(CXX) $(CXXFLAGS) $(BACKUP_TEST_SOURCES) -o $(BACKUP_TEST_TARGET)

$(SYSTEM_INSPECTOR_TEST_TARGET): $(SYSTEM_INSPECTOR_TEST_SOURCES) include/system_inspector.hpp include/localization.hpp
	mkdir -p build
	$(CXX) $(CXXFLAGS) $(SYSTEM_INSPECTOR_TEST_SOURCES) -o $(SYSTEM_INSPECTOR_TEST_TARGET)

$(REPORT_GENERATOR_TEST_TARGET): $(REPORT_GENERATOR_TEST_SOURCES) include/report_generator.hpp include/integrity_checker.hpp include/backup_manager.hpp include/system_inspector.hpp include/localization.hpp
	mkdir -p build
	$(CXX) $(CXXFLAGS) $(REPORT_GENERATOR_TEST_SOURCES) -o $(REPORT_GENERATOR_TEST_TARGET)

$(SETTINGS_TEST_TARGET): $(SETTINGS_TEST_SOURCES) include/settings_manager.hpp include/localization.hpp
	mkdir -p build
	$(CXX) $(CXXFLAGS) $(SETTINGS_TEST_SOURCES) -o $(SETTINGS_TEST_TARGET)

$(LOGGER_TEST_TARGET): $(LOGGER_TEST_SOURCES) include/logger.hpp
	mkdir -p build
	$(CXX) $(CXXFLAGS) $(LOGGER_TEST_SOURCES) -o $(LOGGER_TEST_TARGET)

$(SHA256_TEST_TARGET): $(SHA256_TEST_SOURCES) include/sha256.hpp
	mkdir -p build
	$(CXX) $(CXXFLAGS) $(SHA256_TEST_SOURCES) -o $(SHA256_TEST_TARGET)

$(SWITCH_NRO): source/platform/switch_main.cpp source/system/localization.cpp source/system/settings_manager.cpp source/system/backup_manager.cpp source/system/sha256.cpp include/localization.hpp include/settings_manager.hpp include/backup_manager.hpp include/sha256.hpp
	@if [ -z "$(DEVKITPRO)" ]; then echo "Set DEVKITPRO to build the Switch NRO."; exit 1; fi
	mkdir -p build/switch
	$(SWITCH_CXX) $(SWITCH_FLAGS) \
		-specs=$(DEVKITPRO)/libnx/switch.specs \
		-Wl,--gc-sections \
		source/platform/switch_main.cpp source/system/localization.cpp \
		source/system/settings_manager.cpp source/system/backup_manager.cpp \
		source/system/sha256.cpp \
		-L$(DEVKITPRO)/libnx/lib -lnx -o $(SWITCH_ELF)
	$(DEVKITPRO)/tools/bin/elf2nro $(SWITCH_ELF) $(SWITCH_NRO)

switch: $(SWITCH_NRO)

test: $(INTEGRITY_TEST_TARGET) $(BACKUP_TEST_TARGET) $(SYSTEM_INSPECTOR_TEST_TARGET) $(REPORT_GENERATOR_TEST_TARGET) $(SETTINGS_TEST_TARGET) $(LOGGER_TEST_TARGET) $(SHA256_TEST_TARGET)
	./$(INTEGRITY_TEST_TARGET)
	./$(BACKUP_TEST_TARGET)
	./$(SYSTEM_INSPECTOR_TEST_TARGET)
	./$(REPORT_GENERATOR_TEST_TARGET)
	./$(SETTINGS_TEST_TARGET)
	./$(LOGGER_TEST_TARGET)
	./$(SHA256_TEST_TARGET)

clean:
	rm -f $(TARGET) $(INTEGRITY_TEST_TARGET) $(BACKUP_TEST_TARGET) $(SYSTEM_INSPECTOR_TEST_TARGET) $(REPORT_GENERATOR_TEST_TARGET) $(SETTINGS_TEST_TARGET) $(LOGGER_TEST_TARGET) $(SHA256_TEST_TARGET) $(SWITCH_ELF) $(SWITCH_NRO)

run: all
	./$(TARGET)

.PHONY: all clean run test switch