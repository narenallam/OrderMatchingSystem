#ifndef __CSV_ITERATOR_HPP__
#define __CSV_ITERATOR_HPP__

#include <iterator>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <exception>

namespace NSOrderMatching {
    // each csv row stored in CSVRow object and parsed
    class CSVRow {
        public:
            std::string const& operator[](std::size_t index) const {
                return m_data[index];
            }
            std::size_t size() const {
                return m_data.size();
            }
            void readNextRow(std::istream& str) {
                std::string line;
                std::getline(str, line);

                std::stringstream lineStream(line);
                std::string cell;

                m_data.clear();
                while(std::getline(lineStream, cell, ',')) {
                    m_data.push_back(cell);
                }
            }

            friend std::istream& operator>>(std::istream& str, CSVRow& data) {
                data.readNextRow(str);
                return str;
            }
        private:
            std::vector<std::string> m_data;
    };

    //Utility Iterator class for csv file reading
    class CSVIterator {
        public:
            typedef std::input_iterator_tag     iterator_category;
            typedef CSVRow                      value_type;
            typedef std::size_t                 difference_type;
            typedef CSVRow*                     pointer;
            typedef CSVRow&                     reference;

            CSVIterator(std::istream& str)     :m_str(str.good()?&str:NULL) { ++(*this); }
            CSVIterator()                      :m_str(NULL) {}

            // Pre Increment
            CSVIterator& operator++() {
                if (m_str) {
                    if (!((*m_str) >> m_row)) {
                        m_str = NULL;}
                    }
                    return *this;
                }
            // Post increment
            CSVIterator operator++(int) {
                CSVIterator tmp(*this);
                ++(*this);
                return tmp;
            }

            CSVRow const& operator*() const { return m_row; }
            CSVRow const* operator->() const {return &m_row;}

            bool operator==(CSVIterator const& rhs) {
                return ((this == &rhs) || ((this->m_str == NULL) && (rhs.m_str == NULL)));
            }
            bool operator!=(CSVIterator const& rhs) {
                return !((*this) == rhs);
            }

        private:


            std::istream* m_str;
            CSVRow m_row;
    };
}
#endif
/*
int main()
{
    std::ifstream file("plop.csv");

    for(CSVIterator loop(file); loop != CSVIterator(); ++loop)
    {   try {
            Order ord;
            ord.orderId = 0;
            ord.trader = (*loop)[0];
            ord.stock = (*loop)[1];
            ord.quantity = stoi((*loop)[2]);
            std::string _side{(*loop)[3]};
            std::cout << "Side = "<< _side;
            if (_side.compare("Buy") or _side.compare("Sell")) {
                ord.side = (_side.compare("Buy")) ? TradeSide::Buy : TradeSide::Sell;
            }
            else {
                throw std::runtime_error("Invalid Buy/Sell data");
            }
            ord.status = OrderStatus::Open;
            std::cout<< ord << std::endl;
        }
        catch(std::invalid_argument& ex) {
            std::cout << "Cannot convert from iterator : " << (*loop)[0] << ','
            << (*loop)[1] << ',' << (*loop)[2] << ',' << (*loop)[3] << endl;
            std::cout << ex.what() << endl;
        }
        catch(std::out_of_range& ex) {
            std::cout << "Out of range while order object creation : " << (*loop)[0] << ','
            << (*loop)[1] << ',' << (*loop)[2] << ',' << (*loop)[3] << endl;
            std::cout << ex.what() << endl;
        }
        catch(std::runtime_error &ex){
            std::cout << "Invalid data while object creation : " << (*loop)[0] << ','
            << (*loop)[1] << ',' << (*loop)[2] << ',' << (*loop)[3] << endl;
            std::cout << ex.what() << endl;
        }
    }
    
}
*/

