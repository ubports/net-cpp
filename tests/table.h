/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Thomas Voß <thomas.voss@canonical.com>
 */

#ifndef TABLE_H
#define TABLE_H

#include <iomanip>
#include <sstream>

namespace testing
{
struct Table
{
    Table() = delete;

    template<int field_width, char separator>
    struct Row
    {
        template<int field_count>
        struct HorizontalSeparator
        {
            static constexpr int width = field_count*(2 + field_width + 1) + 1;
        };

        template<int field_count>
        friend std::ostream& operator<<(
            std::ostream& out,
            const HorizontalSeparator<field_count>&)
        {
            out << std::setw(HorizontalSeparator<field_count>::width) << std::setfill('-') << '-' << std::setfill(' ') << std::endl;
            return out;
        }

        struct Field
        {
            static constexpr int width = field_width;

            static const std::string& prefix()
            {
                static const std::string s = std::string{separator} + " ";
                return s;
            }

            static const std::string& suffix()
            {
                static const std::string s{" "};
                return s;
            }
        };

        Row()
        {
            ss << std::setprecision(5) << std::fixed;
        }

        mutable std::stringstream ss;
    };
    
    template<int field_width, char separator, typename T>
    friend Row<field_width, separator>& operator<<(Row<field_width, separator>& row, const T& value)
    {
        row.ss << Row<field_width, separator>::Field::prefix() << std::setw(Row<field_width, separator>::Field::width) << value << Row<field_width, separator>::Field::suffix();

        return row;
    }

    template<int field_width, char separator, typename T>
    friend Row<field_width, separator>& operator<<(Row<field_width, separator>& row, T& value)
    {
        row.ss << Row<field_width, separator>::Field::prefix() << std::setw(Row<field_width, separator>::Field::width) << value << Row<field_width, separator>::Field::suffix();
        
        return row;
    }

    template<int field_width, char separator>
    friend std::ostream& operator<<(std::ostream& out, const Row<field_width, separator>& row)
    {
        out << row.ss.str() << separator << std::endl;
        row.ss.str(std::string{});

        return out;
    }
};
}

#endif // TABLE_H
