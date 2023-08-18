// ct_lvtclp_logicalpostprocessutil.t.cpp                             -*-C++-*-

/*
// Copyright 2023 Codethink Ltd <codethink@codethink.co.uk>
// SPDX-License-Identifier: Apache-2.0
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/

#include <ct_lvtclp_logicalpostprocessutil.h>

#include <ct_lvtmdb_fileobject.h>
#include <ct_lvtmdb_namespaceobject.h>
#include <ct_lvtmdb_objectstore.h>
#include <ct_lvtmdb_typeobject.h>

#include <ct_lvtclp_testutil.h>
#include <ct_lvtshr_graphenums.h>

#include <filesystem>
#include <initializer_list>
#include <string>

#include <catch2-local-includes.h>

namespace {

using namespace Codethink::lvtclp;
using namespace Codethink::lvtmdb;
using namespace Codethink;

std::string fileNameFromQName(std::string qualifiedName)
{
    return std::filesystem::path(std::move(qualifiedName)).filename().string();
}

void checkFileSelection(const std::initializer_list<std::string>& classFiles,
                        std::string className,
                        const std::string& classQName,
                        std::string nmspcName,
                        std::string nmspcQName,
                        const std::string& expectedFileResult)
{
    ObjectStore session;

    session.withRWLock([&] {
        NamespaceObject *obj = session.getOrAddNamespace(nmspcQName, nmspcName, nullptr);
        TypeObject *udt = session.getOrAddType(classQName,
                                               className,
                                               lvtshr::UDTKind::Struct,
                                               lvtshr::AccessSpecifier::e_public,
                                               obj,
                                               nullptr,
                                               nullptr);
        obj->withRWLock([&] {
            obj->addType(udt);
        });

        // set up database
        for (const std::string& fileQName : classFiles) {
            FileObject *file =
                session.getOrAddFile(fileQName, fileNameFromQName(fileQName), true, std::string{}, nullptr, nullptr);
            file->withRWLock([&] {
                file->addType(udt);
            });
            udt->withRWLock([&] {
                udt->addFile(file);
            });
        }
    });

    REQUIRE(LogicalPostProcessUtil::postprocess(session, false));

    TypeObject *udt = nullptr;
    FileObject *expected = nullptr;

    session.withROLock([&] {
        udt = session.getType(classQName);
        expected = session.getFile(expectedFileResult);
    });
    // check results
    REQUIRE(udt);

    udt->withROLock([&] {
        const auto files = udt->files();
        REQUIRE(files.size() == 1);
        REQUIRE(expected);
        REQUIRE(files.front() == expected);
    });
}

} // namespace

TEST_CASE("UsesBslmaAllocator")
{
    std::initializer_list<std::string> files{
        "bdl/bdlc/bdlc_flathashtable.h",
        "bdl/bdlc/bdlc_packedintarray.h",
        "bdl/bdlc/bdlc_bitarray.h",
        "bdl/bdlc/bdlc_flathashmap.h",
        "bdl/bdlt/bdlt_calendar.h",
        "bdl/bdlt/bdlt_packedcalendar.h",
        "bdl/bdlf/bdlf_memfn.h",
        "bdl/bdlcc/bdlcc_cache.h",
        "bdl/bdlcc/bdlcc_stripedunorderedmultimap.h",
        "bdl/bdlcc/bdlcc_stripedunorderedcontainerimpl.h",
        "bsl/bsltf/bsltf_movablealloctesttype.h",
        "bsl/bsltf/bsltf_allocemplacabletesttype.h",
        "bsl/bsltf/bsltf_alloctesttype.h",
        "bsl/bsltf/bsltf_nonoptionalalloctesttype.h",
        "bsl/bsltf/bsltf_allocargumenttype.h",
        "bsl/bsltf/bsltf_wellbehavedmoveonlyalloctesttype.h",
        "bsl/bsltf/bsltf_moveonlyalloctesttype.h",
        "bsl/bsltf/bsltf_allocbitwisemoveabletesttype.h",
        "bsl/bslmt/bslmt_threadgroup.h",
        "bsl/bslalg/bslalg_constructorproxy.h",
        "bsl/bslstl/bslstl_hashtable.h",
        "bsl/bslstl/bslstl_unorderedmap.h",
        "bsl/bslstl/bslstl_unorderedmultimap.h",
        "bsl/bslstl/bslstl_pair.h",
        "bsl/bslstl/bslstl_multimap.h",
        "bsl/bslstl/bslstl_string.h",
        "bsl/bslstl/bslstl_unorderedmultiset.h",
        "bsl/bslstl/bslstl_stringbuf.h",
        "bsl/bslstl/bslstl_sharedptrallocateinplacerep.h",
        "bsl/bslstl/bslstl_set.h",
        "bsl/bslstl/bslstl_istringstream.h",
        "bsl/bslstl/bslstl_stringstream.h",
        "bsl/bslstl/bslstl_unorderedset.h",
        "bsl/bslstl/bslstl_map.h",
        "bsl/bslstl/bslstl_list.h",
        "bsl/bslstl/bslstl_sharedptr.h",
        "bsl/bslstl/bslstl_multiset.h",
        "bsl/bslstl/bslstl_ostringstream.h",
        "bsl/bslstl/bslstl_deque.h",
        "bsl/bslstl/bslstl_vector.h",
        "bsl/bslma/bslma_stdallocator.h",
        "bsl/bslma/bslma_sharedptroutofplacerep.h",
        "bsl/bslma/bslma_sharedptrinplacerep.h",
        "bsl/bslma/bslma_usesbslmaallocator.h",
        "bdl/bdlbb/bdlbb_blob.h",
        "bsl/bslx/bslx_testoutstream.h",
        "bsl/bslx/bslx_byteoutstream.h",
        "bdl/bdlc/bdlc_compactedarray.h",
        "bdl/bdlt/bdlt_timetable.h",
        "bsl/bslstp/bslstp_hashset.h",
        "bbl/bbldc/bbldc_calendardaterangedaycountadapter.h",
        "bdl/bdlde/bdlde_quotedprintableencoder.h",
        "bsl/bslstl/bslstl_boyermoorehorspoolsearcher.h",
        "bsl/bslstp/bslstp_hashmap.h",
        "bbl/bbldc/bbldc_perioddaterangedaycountadapter.h",
        "bdl/bdlt/bdlt_calendarcache.h",
        "bdl/bdlcc/bdlcc_stripedunorderedmap.h",
        "bdl/bdlt/bdlt_timetablecache.h",
        "bdl/bdlma/bdlma_defaultdeleter.h",
        "bdl/bdlc/bdlc_flathashset.h",
        "bdl/bdlcc/bdlcc_deque.h",
    };

    return checkFileSelection(files,
                              "UsesBslmaAllocator",
                              "BloombergLP::bslma::UsesBslmaAllocator",
                              "bslma",
                              "BloombergLP::bslma",
                              "bsl/bslma/bslma_usesbslmaallocator.h");
}

TEST_CASE("IsBitwiseMoveable")
{
    std::initializer_list<std::string> files{
        "bdl/bdlc/bdlc_bitarray.h",
        "bdl/bdlt/bdlt_dayofweekset.h",
        "bdl/bdlf/bdlf_memfn.h",
        "bsl/bslh/bslh_siphashalgorithm.h",
        "bsl/bslh/bslh_hash.h",
        "bsl/bslh/bslh_spookyhashalgorithm.h",
        "bsl/bslmf/bslmf_isbitwisemoveable.h",
        "bsl/bsltf/bsltf_allocbitwisemoveabletesttype.h",
        "bsl/bsltf/bsltf_bitwisemoveabletesttype.h",
        "bsl/bslalg/bslalg_constructorproxy.h",
        "bsl/bslstl/bslstl_hashtable.h",
        "bsl/bslstl/bslstl_unorderedmap.h",
        "bsl/bslstl/bslstl_unorderedmultimap.h",
        "bsl/bslstl/bslstl_pair.h",
        "bsl/bslstl/bslstl_bidirectionalnodepool.h",
        "bsl/bslstl/bslstl_list.h",
        "bsl/bslstl/bslstl_sharedptr.h",
        "bsl/bslstl/bslstl_deque.h",
        "bsl/bslstl/bslstl_vector.h",
        "bsl/bslma/bslma_managedptr_members.h",
        "bsl/bslma/bslma_managedptrdeleter.h",
        "bsl/bslma/bslma_managedptr.h",
        "bsl/bslma/bslma_managedptr_pairproxy.h",
        "bdl/bdlbb/bdlbb_blob.h",
        "bsl/bslh/bslh_wyhashalgorithm.h",
    };

    return checkFileSelection(files,
                              "IsBitwiseMoveable",
                              "BloombergLP::bslmf::IsBitwiseMoveable",
                              "bslmf",
                              "BloombergLP::bslmf",
                              "bsl/bslmf/bslmf_isbitwisemoveable.h");
}

TEST_CASE("IsTriviallyCopyable")
{
    std::initializer_list<std::string> files{
        "bdl/bdlt/bdlt_datetimeinterval.h",
        "bdl/bdlt/bdlt_date.h",
        "bdl/bdlt/bdlt_timetz.h",
        "bdl/bdlt/bdlt_time.h",
        "bdl/bdlt/bdlt_datetimetz.h",
        "bdl/bdlt/bdlt_datetime.h",
        "bdl/bdlt/bdlt_datetz.h",
        "bdl/bdlf/bdlf_memfn.h",
        "bsl/bslh/bslh_hash.h",
        "bsl/bslmf/bslmf_istriviallycopyable.h",
        "bsl/bslalg/bslalg_dequeiterator.h",
        "bsl/bslstl/bslstl_pair.h",
        "bsl/bslstl/bslstl_equalto.h",
        "bsl/bslstl/bslstl_hash.h",
        "bsl/bslstl/bslstl_ownerless.h",
        "bsl/bslstl/bslstl_typeindex.h",
    };

    return checkFileSelection(files,
                              "is_trivially_copyable",
                              "bsl::is_trivially_copyable",
                              "bsl",
                              "bsl",
                              "bsl/bslmf/bslmf_istriviallycopyable.h");
}
