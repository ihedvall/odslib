/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */
#include <algorithm>
#include <string_view>
#include <array>
#include <vector>
#include "ods/odsdef.h"
#include "ods/baseattribute.h"
#include "util/stringutil.h"

using namespace util::string;
namespace {

struct BaseIdDef {
  const int BaseId;
  const std::string_view BaseName1;
  const std::string_view BaseName2;
};

constexpr std::array<BaseIdDef,31> kBaseIdList = {
    BaseIdDef{0, "AoAny", "Any"},
    BaseIdDef{1, "AoEnvironment", "Environment"},
    BaseIdDef{2, "AoSubTest", "SubTest"},
    BaseIdDef{3, "AoMeasurement", "Measurement"},
    BaseIdDef{4, "AoMeasurementQuantity", "MeasurementQuantity"},
    BaseIdDef{11, "AoQuantity", "Quantity"},
    BaseIdDef{12, "AoQuantityGroup", "QuantityGroup"},
    BaseIdDef{13, "AoUnit", "Unit"},
    BaseIdDef{14, "AoUnitGroup", "UnitGroup"},
    BaseIdDef{15, "AoPhysicalDimension", "PhysicalDimension"},
    BaseIdDef{21, "AoUnitUnderTest", "UnitUnderTest"},
    BaseIdDef{22, "AoUnitUnderTestPart", "UnitUnderTestPart"},
    BaseIdDef{23, "AoTestEquipment", "TestEquipment"},
    BaseIdDef{24, "AoTestEquipmentPart", "TestEquipmentPart"},
    BaseIdDef{25, "AoTestSequence", "TestSequence"},
    BaseIdDef{26, "AoTestSequencePart", "TestSequencePart"},
    BaseIdDef{34, "AoUser", "User"},
    BaseIdDef{35, "AoUserGroup", "UserGroup"},
    BaseIdDef{36, "AoTest", "Test"},
    BaseIdDef{37, "AoTestDevice", "TestDevice"},
    BaseIdDef{38, "AoSubMatrix", "SubMatrix"},
    BaseIdDef{39, "AoLocalColumn", "LocalColumn"},
    BaseIdDef{40, "AoExternalComponent", "ExternalComponent"},
    BaseIdDef{43, "AoLog", "Log"},
    BaseIdDef{44, "AoParameter", "Parameter"},
    BaseIdDef{45, "AoParameterSet", "ParameterSet"},
    BaseIdDef{46, "AoNameMap", "NameMap"},
    BaseIdDef{47, "AoAttributeMap", "AttributeMap"},
    BaseIdDef{48, "AoFile", "File"},
    BaseIdDef{49, "AoMimetypeMap", "MimetypeMap"},
    BaseIdDef{255, "AoNotDefined", "NotDefined"},
};

struct DataTypeDef {
  const int DataType;
  const std::string_view TypeName1;
  const std::string_view TypeName2;
  const std::string_view TypeName3;
};

constexpr std::array<DataTypeDef,31> kDataTypeList = {
    DataTypeDef{0, "DtUnknown", "DT_UNKNOWN", "Unknown"},
    DataTypeDef{1, "DtString", "DT_STRING", "String"},
    DataTypeDef{2, "DtShort", "DT_SHORT", "Short"},
    DataTypeDef{3, "DtFloat", "DT_FLOAT", "Float"},
    DataTypeDef{4, "DtBoolean", "DT_BOOLEAN", "Boolean"},
    DataTypeDef{5, "DtByte", "DT_BYTE", "Byte"},
    DataTypeDef{6, "DtLong", "DT_LONG", "Long"},
    DataTypeDef{7, "DtDouble", "DT_DOUBLE", "Double"},
    DataTypeDef{8, "DtLongLong", "DT_LONGLONG", "LongLong"},
    DataTypeDef{9, "DtId", "DT_ID", "Id"},
    DataTypeDef{10, "DtDate", "DT_DATE", "Date"},
    DataTypeDef{11, "DtByteString", "DT_BYTESTRING", "ByteString"},
    DataTypeDef{12, "DtBlob", "DT_BLOB", "Blob"},
    DataTypeDef{13, "DtComplex", "DT_COMPLEX", "Complex"},
    DataTypeDef{14, "DtDComplex", "DT_DCOMPLEX", "DComplex"},

    DataTypeDef{15, "DsString", "DS_STRING", "StringSeq"},
    DataTypeDef{16, "DsShort", "DS_SHORT", "ShortSeq"},
    DataTypeDef{17, "DsFloat", "DS_FLOAT", "FloatSeq"},
    DataTypeDef{18, "DsBoolean", "DS_BOOLEAN", "BooleanSeq"},
    DataTypeDef{19, "DsByte", "DS_BYTE", "ByteSeq"},
    DataTypeDef{20, "DsLong", "DS_LONG", "LongSeq"},
    DataTypeDef{21, "DsDouble", "DS_DOUBLE", "DoubleSeq"},
    DataTypeDef{22, "DsLongLong", "DS_LONGLONG", "LongLongSeq"},
    DataTypeDef{23, "DsComplex", "DS_COMPLEX", "ComplexSeq"},
    DataTypeDef{24, "DsDComplex", "DS_DCOMPLEX", "DComplexSeq"},
    DataTypeDef{25, "DsId", "DS_ID", "IdSeq"},
    DataTypeDef{26, "DsDate", "DS_DATE", "DateSeq"},
    DataTypeDef{27, "DsByteString", "DS_BYTESTRING", "ByteStringSeq"},

    DataTypeDef{28, "DtExternalRef", "DT_EXTERNALREF", "ExternalRef"},
    DataTypeDef{30, "DtEnum", "DT_ENUM", "Enum"},
    DataTypeDef{31, "DsEnum", "DS_ENUM", "EnumSeq"},
};

struct TypeSpecDef {
  const int Type;
  const std::string_view TypeName;
};

constexpr std::array<TypeSpecDef, 34>  kTypeSpecList = {
    TypeSpecDef{0, "dt_boolean"},
    TypeSpecDef{1, "dt_byte"},
    TypeSpecDef{2, "dt_short"},
    TypeSpecDef{3, "dt_long"},
    TypeSpecDef{4, "dt_longlong"},
    TypeSpecDef{5, "ieeefloat4"},
    TypeSpecDef{6, "ieeefloat8"},
    TypeSpecDef{7, "dt_short_beo"},
    TypeSpecDef{8, "dt_long_beo"},
    TypeSpecDef{9, "dt_longlong_beo"},
    TypeSpecDef{10, "ieeefloat4_beo"},
    TypeSpecDef{11, "ieeefloat8_beo"},
    TypeSpecDef{12, "dt_string"},
    TypeSpecDef{13, "dt_bytestr"},
    TypeSpecDef{14, "dt_blob"},
    TypeSpecDef{15, "dt_boolean_flags_beo"},
    TypeSpecDef{16, "dt_byte_flags_beo"},
    TypeSpecDef{17, "dt_string_flags_beo"},
    TypeSpecDef{18, "dt_bytestr_beo"},
    TypeSpecDef{19, "dt_sbyte"},
    TypeSpecDef {20, "dt_sbyte_flags_beo"},
    TypeSpecDef {21, "dt_ushort"},
    TypeSpecDef{22, "dt_ushort_beo"},
    TypeSpecDef {23, "dt_ulong"},
    TypeSpecDef{24, "dt_ulong_beo"},
    TypeSpecDef{25, "dt_string_utf8"},
    TypeSpecDef{26, "dt_string_utf8_flags_beo"},
    TypeSpecDef{27, "dt_bit_int"},
    TypeSpecDef{28, "dt_bit_int_beo"},
    TypeSpecDef{29, "dt_bit_uint"},
    TypeSpecDef{30, "dt_bit_uint_beo"},
    TypeSpecDef {31, "dt_bit_ieeefloat"},
    TypeSpecDef{32, "dt_bit_ieeefloat_beo"},
    TypeSpecDef{33, "dt_bytestr_leo"},
};

struct SeqDef {
  const int Type;
  const std::string_view TypeName;
};
constexpr std::array<SeqDef,14> kSequenceList = {
    SeqDef{0, "explicit"},
    SeqDef{1, "implicit_constant"},
    SeqDef{2, "implicit_linear"},
    SeqDef{3, "implicit_saw"},
    SeqDef{4, "raw_linear"},
    SeqDef{5, "raw_polynomial"},
    SeqDef{6, "formula"},
    SeqDef{7,"external_component"},
    SeqDef{8,"raw_linear_external"},
    SeqDef{9,"raw_polynomial_external"},
    SeqDef{10, "raw_linear_calibrated"},
    SeqDef{11, "raw_linear_calibrated_external"},
    SeqDef{12, "raw_rational"},
    SeqDef{13, "raw_rational_external"},
};

struct EnvTypeDef {
  const int EnvType;
  const std::string_view TypeName1; ///< Computer text name
  const std::string_view TypeName2; ///< Display text
};

constexpr std::array<EnvTypeDef,2> kEnvTypeList = {
    EnvTypeDef{1, "Generic", "Generic ODS Environment"},
    EnvTypeDef{2, "TestDirectory", "Test Directory Environment"},
};


} // end namespace

namespace ods {

BaseId TextToBaseId(const std::string &text) {
  int temp = -1;
  try {
    temp = std::stoi(text);
  } catch(const std::exception&) {
    // Normal if not an integer
  }
  const auto itr = std::ranges::find_if(kBaseIdList, [&] (const auto& base) {
      return temp == base.BaseId ||IEquals(text,base.BaseName1.data()) ||
          IEquals(text,base.BaseName2.data());
  });
  return itr == kBaseIdList.cend() ? BaseId::AoNotDefined : static_cast<BaseId>(itr->BaseId);
}

std::string BaseIdToText(BaseId base) {
  const int temp = static_cast<int> (base);
  const auto itr = std::ranges::find_if(kBaseIdList, [&] (const auto& base) {
    return temp == base.BaseId;
  });
  return itr == kBaseIdList.cend() ? "": itr->BaseName1.data();
}

std::string BaseIdToUserText(BaseId base) {
  const int temp = static_cast<int> (base);
  const auto itr = std::ranges::find_if(kBaseIdList, [&] (const auto& base) {
    return temp == base.BaseId;
  });
  return itr == kBaseIdList.cend() ? "": itr->BaseName2.data();
}

DataType TextToDataType(const std::string &text) {
  int temp = -1;
  try {
    temp = std::stoi(text);
  } catch(const std::exception&) {
    // Normal if not an integer
  }

  for (const auto type : kDataTypeList) {
    if (temp == type.DataType ||
        IEquals(text,type.TypeName1.data()) ||
        IEquals(text,type.TypeName2.data()) ||
        IEquals(text,type.TypeName3.data())) {
      return static_cast<DataType>(type.DataType);
    }
  }
  return DataType::DtUnknown;
}

std::string DataTypeToText(DataType type) {
  const int temp = static_cast<int> (type);

  for (const auto tid : kDataTypeList) {
    if (temp == tid.DataType) {
      return tid.TypeName1.data();
    }
  }
  return "";
}

std::string DataTypeToUserText(DataType type) {
  const int temp = static_cast<int> (type);

  for (const auto tid : kDataTypeList) {
    if (temp == tid.DataType) {
      return tid.TypeName3.data();
    }
  }
  return "";
}

IEnum CreateDefaultEnum(const std::string &enum_name) {
  IEnum enumerate;
  enumerate.EnumName(enum_name);
  enumerate.Locked(true);
  if (IEquals(enum_name, "ao_storagetype_enum")) {
    enumerate.AddItem(0, "database");
    enumerate.AddItem(1, "external_only");
    enumerate.AddItem(2, "mixed");
    enumerate.AddItem(3, "foreign_format");
  } else if (IEquals(enum_name, "datatype_enum")) {
    for (const auto& type : kDataTypeList) {
      if ( (type.DataType >= 0 && type.DataType <= 14) ||
           type.DataType == 28 || type.DataType == 30) {
        enumerate.AddItem(type.DataType, type.TypeName2.data());
      }
    }
  } else if (IEquals(enum_name, "interpolation_enum")) {
    enumerate.AddItem(0, "no_interpolation");
    enumerate.AddItem(1, "linear_interpolation");
    enumerate.AddItem(2, "application_specific");
  } else if (IEquals(enum_name, "seq_rep_enum")) {
    for (const auto& type : kSequenceList) {
      enumerate.AddItem(type.Type, type.TypeName.data());
    }
  } else if (IEquals(enum_name, "typespec_enum")) {
    for (const auto& type : kTypeSpecList) {
      enumerate.AddItem(type.Type, type.TypeName.data());
    }
  }
  return enumerate;
}

std::string EnvTypeToText(EnvironmentType type) {
  const int temp = static_cast<int> (type);
  const auto itr = std::ranges::find_if(kEnvTypeList, [&] (const auto& itr) {
    return temp == itr.EnvType;
  });
  return itr == kEnvTypeList.cend() ? "": itr->TypeName1.data();
}

std::string EnvTypeToUserText(EnvironmentType type) {
  const int temp = static_cast<int> (type);
  const auto itr = std::ranges::find_if(kEnvTypeList, [&] (const auto& itr) {
    return temp == itr.EnvType;
  });
  return itr == kEnvTypeList.cend() ? "": itr->TypeName2.data();
}

EnvironmentType TextToEnvType(const std::string &text) {
  int temp = -1;
  try {
    temp = std::stoi(text);
  } catch(const std::exception&) {
    // Normal if not an integer
  }

  for (const auto type : kEnvTypeList) {
    if (temp == type.EnvType ||
        IEquals(text,type.TypeName1.data()) ||
        IEquals(text,type.TypeName2.data()) ) {
      return static_cast<EnvironmentType>(type.EnvType);
    }
  }
  return EnvironmentType::kTypeUnknown;
}

} // end namespace

