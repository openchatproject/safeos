#!/usr/bin/env python3

import json
import sys
import os.path
import fnmatch

def add_ricardian_contracts_to_actions(source_abi_directory, contract_name, abi_actions):
    abi_actions_with_ricardian_contracts = []

    for abi_action in abi_actions:
        action_name = abi_action["name"]
        contract_action_filename = f'{contract_name}-{action_name}-rc.md'
        
        # check for rc file
        rcContractPath = os.path.join(source_abi_directory, contract_action_filename)
        if os.path.exists(rcContractPath):
            print('Importing Contract {contract_action_filename} for {contract_name}:{action_name}'.format(
                contract_action_filename = contract_action_filename, 
                contract_name = contract_name, 
                action_name = action_name
            ))

            with open(rcContractPath) as contract_file_handle:
                contract_contents = contract_file_handle.read()

            abi_action["ricardian_contract"] = contract_contents
        else:
            print('Did not find recardian contract file {contract_action_filename} for {contract_name}:{action_name}, skipping inclusion'.format(
                contract_action_filename = contract_action_filename,
                contract_name = contract_name,
                action_name = action_name
            ))

        abi_actions_with_ricardian_contracts.append(abi_action)
    
    return abi_actions_with_ricardian_contracts

def create_ricardian_clauses_list(source_abi_directory, contract_name):
    clause_file_pattern = '*-clause*-rc.md'
    clause_files = fnmatch.filter(os.listdir(source_abi_directory), clause_file_pattern)

    clause_prefix = 'clause-'
    clause_postfix = '-rc.md'

    abi_ricardian_clauses = []

    for clause_file_name in clause_files:
        rcContractPath = os.path.join(source_abi_directory, clause_file_name)
        with open(rcContractPath) as contract_file_handle:
            contract_contents = contract_file_handle.read()

        start_of_clause_id = clause_file_name.index( clause_prefix ) + len( clause_prefix )
        end_of_clause_id = clause_file_name.rindex(clause_postfix, start_of_clause_id)

        clause_id = clause_file_name[start_of_clause_id:end_of_clause_id]

        abi_ricardian_clauses.append({
            'id': clause_id, 
            'body': contract_contents
        })

    return abi_ricardian_clauses

def add_ricardian_contracts_to_abi(source_abi, output_abi):
    source_abi_directory = os.path.dirname(source_abi)
    contract_name = os.path.split(source_abi)[1].rpartition(".")[0]

    print('Creating {output_abi} with ricardian contracts included'.format(output_abi = output_abi))

    with open(source_abi, 'r') as source_abi_file:
        source_abi_json = json.load(source_abi_file)

    source_abi_json['actions'] = add_ricardian_contracts_to_actions(source_abi_directory, contract_name, source_abi_json['actions'])
    source_abi_json['ricardian_clauses'] = create_ricardian_clauses_list(source_abi_directory, contract_name)

    with open(output_abi, 'w') as output_abi_file:
        json.dump(source_abi_json, output_abi_file, indent=2)


def import_ricardian_to_abi(source_abi, output_abi):
    if not os.path.exists(source_abi):
        print(f'Source ABI not found in {source_abi}')
        sys.exit(0)

    if os.path.exists(output_abi):
        overwrite_prompt_response = input('Output ABI {output_abi} already exists, do you want to proceed? (y|n): '.format(output_abi = output_abi))
        if  overwrite_prompt_response == 'y':
            print('Overwriting existing output abi')
            add_ricardian_contracts_to_abi(source_abi, output_abi)
            sys.exit(0)
        else:
            print('User aborted, not overwriting existing abi')
            sys.exit(0)
    else:
        add_ricardian_contracts_to_abi(source_abi, output_abi)
    
def main():
    if len(sys.argv) == 1:
        print('Please specify an operation of export or import: ./ricardeos.py <import|export>')
        sys.exit(1)

    if sys.argv[1] == 'import':
        if len(sys.argv) != 4:
            print('Please specify a source and destination abi:')
            print('Usage: ./ricardeos.py import /eos/contracts/contract/mycontract.abi /eos/contracts/contract/withricardian-mycontract.abi')

            sys.exit(0)
        else:
            import_ricardian_to_abi(sys.argv[2], sys.argv[3])

            sys.exit(0)
    elif sys.argv[2] == 'export':
        print('exporting')
    else:
        print('Operation not recognized only import and export operations are supported')

if __name__ == '__main__':
        main()

