"""
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
"""

import click
import string
from random import sample

# Constants
SAMPLE_DATA = '''Trader_2,Stock_X,500,Buy
Trader_3,Stock_X,700,Buy
Trader_5,Stock_X,1000,Sell
Trader_5,Stock_X,200,Sell
Trader_1,Stock_Y,1000,Buy
Trader_4,Stock_Y,1100,Sell
Trader_2,Stock_Y,100,Buy
Trader_5,Stock_Z,1000,Sell
Trader_2,Stock_Z,200,Buy
Trader_4,Stock_Z,800,Buy'''

STOCK_TYPE_COUNT = 3
TRADER_TYPE_COUNT = 5
MAX_QUANTITY = 1000
STOCKS = ['Stock_' + chr(65 + i) for i in range(25, 25 - STOCK_TYPE_COUNT, -1)]

# Helper functions
def generate_random_orders(count, file):
    """Generate random buy/sell orders with random stocks, quantities, and traders"""
    traders = [f'Trader_{i+1}' for i in range(TRADER_TYPE_COUNT + 1)]
    quantities = [str(x) for x in range(100, MAX_QUANTITY + 1, 100)]
    sides = ['Buy', 'Sell']
    
    for _ in range(count):
        trader = sample(traders, 1)[0]
        stock = sample(STOCKS, 1)[0]
        quantity = sample(quantities, 1)[0]
        side = sample(sides, 1)[0]
        file.write(f"{trader},{stock},{quantity},{side}\n")

def generate_flood_orders(count, file):
    """Generate flood orders for load testing - many buys and one big sell"""
    # Use all possible trader names to avoid running out of traders
    traders = [f'Trader_{c}' for c in string.ascii_uppercase]
    
    # Generate 'count' buy orders, cycling through traders as needed
    for i in range(count):
        trader_idx = i % len(traders)
        file.write(f"{traders[trader_idx]},Stock_X,1,Buy\n")
    
    # Add the final sell order matching all buys
    file.write(f'Trader_Z,Stock_X,{count},Sell\n')

def generate_sample_data(file):
    """Generate sample data for testing"""
    file.write(SAMPLE_DATA)

@click.group()
def cli():
    """Order Matching System - Data Generator

    This tool generates test data for the Order Matching System.
    """
    pass

@cli.command('random')
@click.argument('count', type=click.INT, default=10)
def random_cmd(count):
    """Generate random buy/sell orders.
    
    COUNT is the number of records to generate (default: 10)
    """
    with open('orders.csv', 'w') as f:
        generate_random_orders(count, f)
    click.echo(f"Success: orders.csv generated with {count} random records.")

@cli.command('flood')
@click.argument('count', type=click.INT, default=10)
def flood_cmd(count):
    """Generate flood orders for load testing.
    
    COUNT is the number of records to generate (default: 10)
    """
    with open('orders.csv', 'w') as f:
        generate_flood_orders(count, f)
    click.echo(f"Success: orders.csv generated with {count} single stock records for load testing.")

@cli.command('sample')
def sample_cmd():
    """Generate sample data for testing."""
    with open('orders.csv', 'w') as f:
        generate_sample_data(f)
    click.echo("Success: sample orders.csv generated!")

if __name__ == '__main__':
    try:
        cli()
    except Exception as e:
        click.echo(f"Error: {e}", err=True)
