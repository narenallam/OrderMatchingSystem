
"""
------------------------------------------------------------------------------
File: 
    DataGenerator.py
Author:
    Narendra Allam
Description: 
    orders.csv data generator
    Generates 'random' or 'flood' type of Data or a sample orders.csv file
    random - this is default
    flood - generates data for single stock(Stock_X),
            this is for load testing.
Syntax:
    1. python DataGenerator.py [number of records] [-flood]
    2. python DataGenerator.py -sample
Usage :
    e.g,
    $ python DataGenerator.py 100000 flood 
    - above command generates orders.csv a file with 100000 records all 'Buy's of qty 1 and one 'Sell' of Stock_X
    $ python DataGenerator.py 10
    - above command generates orders.csv with random Buy and Sell of Randome Quantity
    $ python DataGenerator.py sample
    - above command generates sample data of 10 orders and creats orders.csv

Data Intrepretation:
------------------------------------------------------------------------------
Traders    :- Trader_1, Trader_2,....no limit
Stocks     :- Stocks_A, Stock_B,...Stock_Z
Quantities :- 100, 200, 3000 - 10000
Side       :- Buy or Sell
------------------------------------------------------------------------------
"""

from random import sample
import sys

SAMPLE_DATA ='''Trader_2,Stock_X,500,Buy
Trader_3,Stock_X,700,Buy
Trader_5,Stock_X,1000,Sell
Trader_5,Stock_X,200,Sell
Trader_1,Stock_Y,1000,Buy
Trader_4,Stock_Y,1100,Sell
Trader_2,Stock_Y,100,Buy
Trader_5,Stock_Z,1000,Sell
Trader_2,Stock_Z,200,Buy
Trader_4,Stock_Z,800,Buy'''

# STOCK_TYPE_COUNT = 5 # This generates Stock_Z to Stock_V
STOCK_TYPE_COUNT = 3
TRADER_TYPE_COUNT = 5
# MAX_QAUNTITY = 100000 # Max quantity for one order
MAX_QAUNTITY = 1000
STOCKS = ['Stock_'+chr(65+i) for i in range(25,  25-STOCK_TYPE_COUNT, -1)]
def main():
    record_count = 10
    flood = False
    if len(sys.argv) > 4 :
        print("Invalid number of options: %s" ,len(sys.argv))
        usage()

    if len(sys.argv) > 2 :
            if (sys.argv[2].lower() == '-flood'):
                flood = True
            else:
                print("Invalid option: " ,sys.argv[2])
                print("Error: orders.csv cannot be generated!")
                usage()
                return

    if len(sys.argv) > 1:
        if sys.argv[1].lower() == '-sample':
            with open('orders.csv', 'w') as f:
                f.write(SAMPLE_DATA)
                print("Success: sample orders.csv generated!")
                return
        try : 
            record_count = int(sys.argv[1])
        except Exception as ex:
            print("Invalid count = ", sys.argv[1], ", exception : ", str(ex))
            print("Error: orders.csv cannot be generated!")
            usage()
            return

    with open('orders.csv', 'w')as f:
        if not flood:
            for i in range(record_count):
                f.write(sample(['Trader_'+str(i+1) for i in range(TRADER_TYPE_COUNT+1)], 1)[0]+','+sample(STOCKS, 1)[0]+','+
                sample([str(x) for x in range(100, MAX_QAUNTITY+1, 100)],1)[0]+','+sample(['Buy', 'Sell'], 1)[0]+'\n')
                print("Success: orders.csv generated! with %s random records."%record_count)
        else:
            for i in range(record_count):
                f.write(sample(['Trader_'+chr(65+i) for i in range(26)], 1)[0]+ ','+'Stock_X'+','+ '1'+','+'Buy'+'\n')
            f.write('Trader_Z'+ ','+'Stock_X'+','+ str(record_count)+','+'Sell')
            print("Success: orders.csv generated! with %s single stock records."%record_count)
       

def usage():
    print('''
Syntax:
    1. python DataGenerator.py [number ofrecords] [-flood]
    2. python DataGenerator.py -sample
Usage:
    e.g,
    $ python DataGenerator.py 100000 flood 
    - above command generates orders.csv a file with 100000 records all 'Buy's of qty 1 and one 'Sell' of Stock_X
    $ python DataGenerator.py 10
    - above command generates orders.csv with random Buy and Sell of Randome Quantity
    $ python DataGenerator.py sample
    - above command generates sample data of 10 orders and creats orders.csv''')
if __name__ == '__main__': main()