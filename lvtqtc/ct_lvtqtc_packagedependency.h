// ct_lvtqtc_packagedependency.h                                    -*-C++-*-

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

#ifndef INCLUDED_CT_LVTGRPS_PACKAGEDEPENDENCY
#define INCLUDED_CT_LVTGRPS_PACKAGEDEPENDENCY

#include <lvtqtc_export.h>

#include <QPainter>

#include <ct_lvtqtc_lakosrelation.h>

namespace Codethink::lvtqtc {

class PackageDependencyTestFriendAdapter;

/*! \class PackageDependency package_dependency.cpp package_dependency.h
 *  \brief Represents and draws a PackageDependency LakosRelation
 *
 * %PackageDependency draws an Package-Dependency LakosRelation
 */
class LVTQTC_EXPORT PackageDependency : public LakosRelation {
  public:
    friend class PackageDependencyTestFriendAdapter;
    enum class PackageDependencyConstraint : int {
        Unknown = 0x00,
        AllowedDependency = 0x01,
        ConcreteDependency = 0x02,
        ConcreteAndAllowed = AllowedDependency | ConcreteDependency
    };

    /*! \brief Constructs an PackageDependency relation
     *
     * The source vertex is where the arrow goes
     */
    PackageDependency(LakosEntity *source, LakosEntity *target);
    PackageDependency(PackageDependency const& other) = delete;

    /*! \brief The relationType of a particular instance of sub class LakosRelation
     */
    [[nodiscard]] lvtshr::LakosRelationType relationType() const override;
    [[nodiscard]] std::string relationTypeAsString() const override;

    void updateFlavor();
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

  protected:
    enum class PackageDependencyFlavor {
        Unknown,
        ConcreteDependency,
        TestOnlyConcreteDependency,
        AllowedDependency,
        InvalidDesignDependency
    };
    friend constexpr PackageDependencyConstraint operator&(PackageDependencyConstraint x,
                                                           PackageDependencyConstraint y);
    friend constexpr PackageDependencyConstraint operator|(PackageDependencyConstraint x,
                                                           PackageDependencyConstraint y);

    void setFlavor(PackageDependencyFlavor newFlavor);
    [[nodiscard]] QColor hoverColor() const override;

  private:
    PackageDependencyFlavor d_flavor = PackageDependencyFlavor::Unknown;
};

inline constexpr PackageDependency::PackageDependencyConstraint
operator&(PackageDependency::PackageDependencyConstraint x, PackageDependency::PackageDependencyConstraint y)
{
    return static_cast<PackageDependency::PackageDependencyConstraint>(static_cast<int>(x) & static_cast<int>(y));
}

inline constexpr PackageDependency::PackageDependencyConstraint
operator|(PackageDependency::PackageDependencyConstraint x, PackageDependency::PackageDependencyConstraint y)
{
    return static_cast<PackageDependency::PackageDependencyConstraint>(static_cast<int>(x) | static_cast<int>(y));
}

} // end namespace Codethink::lvtqtc

#endif
