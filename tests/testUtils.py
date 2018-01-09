import subprocess
import time
import glob
import shutil
import time
import os
from collections import namedtuple
import re
import string
import signal
import time
import datetime
import inspect
import sys
import random
import io

###########################################################################################
class Utils:
    Debug=False
    FNull = open(os.devnull, 'w')
    
    @staticmethod
    def Print(*args, **kwargs):
        stackDepth=len(inspect.stack())-2
        str=' '*stackDepth
        sys.stdout.write(str)
        print(*args, **kwargs)

    SyncStrategy=namedtuple("ChainSyncStrategy", "name id arg")

    SyncNoneTag="none"
    SyncReplayTag="replay"
    SyncResyncTag="resync"

    SigKillTag="kill"
    SigTermTag="term"
    
    @staticmethod
    def getChainStrategies():
        chainSyncStrategies={}
    
        chainSyncStrategy=Utils.SyncStrategy(Utils.SyncNoneTag, 0, "")
        chainSyncStrategies[chainSyncStrategy.name]=chainSyncStrategy

        chainSyncStrategy=Utils.SyncStrategy(Utils.SyncReplayTag, 1, "--replay-blockchain")
        chainSyncStrategies[chainSyncStrategy.name]=chainSyncStrategy

        chainSyncStrategy=Utils.SyncStrategy(Utils.SyncResyncTag, 2, "--resync-blockchain")
        chainSyncStrategies[chainSyncStrategy.name]=chainSyncStrategy

        return chainSyncStrategies

###########################################################################################
class Transaction(object):
    def __init__(self, transId):
        self.transId=transId
        self.tType=None
        self.amount=0
        
###########################################################################################
class Account(object):
    def __init__(self, name):
        self.name=name
        self.balance=0
        
        self.ownerPrivateKey=None
        self.ownerPublicKey=None
        self.activePrivateKey=None
        self.activePublicKey=None


    def __str__(self):
        return "Name; %s" % (self.name)

###########################################################################################
class Node(object):
    
    def __init__(self, host, port, pid, cmd, alive):
        self.host=host
        self.port=port
        self.pid=pid
        self.cmd=cmd
        self.alive=alive
        self.endpointArgs="--host %s --port %d" % (self.host, self.port)

    def __str__(self):
        return "Port:%d, Pid:%d, Alive:%s, Cmd:\"%s\"" % (self.port, self.pid, self.alive, self.cmd)

    def setWalletEndpointArgs(self, args):
        self.endpointArgs="--host %s --port %d %s" % (self.host, self.port, args)
        
    def doesNodeHaveBlockNum(self, blockNum):
        if self.alive is False:
            return False
    
        Utils.Debug and Utils.Print("Request block num %s from node on port %d" % (blockNum, self.port))
        cmd="programs/eosc/eosc %s get block %s" % (self.endpointArgs, blockNum)
        Utils.Debug and Utils.Print("cmd: %s" % (cmd))
        if 0 != subprocess.call(cmd.split(), stdout=Utils.FNull, stderr=Utils.FNull):
            Utils.Debug and Utils.Print("Block num %s request failed: " % (blockNum))
            return False

        return True

    def doesNodeHaveTransId(self, transId):
        if self.alive is False:
            return False
    
        Utils.Debug and Utils.Print("Request transation id %s from node on port %d" % (transId, self.port))
        cmd="programs/eosc/eosc %s get transaction %s" % (self.endpointArgs, transId)
        Utils.Debug and Utils.Print("cmd: %s" % (cmd))
        if 0 != subprocess.call(cmd.split(), stdout=Utils.FNull, stderr=Utils.FNull):
            Utils.Debug and Utils.Print("Transaction id %s request failed: " % (transId))
            return False

        return True

    # Create account and return creation transactions
    # waitForTransBlock: wait on creation transaction id to appear in a block
    def createAccount(self, account, creatorAccount, waitForTransBlock=False):
        transId=None
        p = re.compile('\n\s+\"transaction_id\":\s+\"(\w+)\",\n', re.MULTILINE)
        cmd="programs/eosc/eosc %s create account %s %s %s %s" % (self.endpointArgs,
            creatorAccount.name, account.name,
            account.ownerPublicKey, account.activePublicKey)
        Utils.Debug and Utils.Print("cmd: %s" % (cmd))
        try:
            retStr=subprocess.check_output(cmd.split(), stderr=subprocess.STDOUT).decode("utf-8")
            # Utils.Print("Ret: ", retStr)
            m=p.search(retStr)
            if m is None:
                Utils.Print("ERROR: transaction id parser failure")
                return None
            transId=m.group(1)
            # Utils.Print("Transaction num: %s" % (transId))
       
        except Exception as ex:
            msg=ex.output.decode("utf-8")
            Utils.Print("ERROR: Exception during account creation. %s" % (msg))
            return None

        if waitForTransBlock and not self.waitForTransIdOnNode(transId):
            return None
        return transId

    def verifyAccount(self, account):
        try :
            cmd="programs/eosc/eosc %s get account %s" % (self.endpointArgs, account.name)
            Utils.Debug and Utils.Print("cmd: %s" % (cmd))
            ret=subprocess.check_output(cmd.split()).decode("utf-8")
            #Utils.Print ("ret:", ret)
            p=re.compile('\n\s+\"staked_balance\"\:\s+\"\d+.\d+\s+EOS\",\n', re.MULTILINE)
            m=p.search(ret)
            if m is None:
                Utils.Print("ERROR: Failed to verify account creation.", account.name)
                return False
        except Exception as es:
            msg=ex.output.decode("utf-8")
            Utils.Print("ERROR: Exception during account verification. %s", msg)
            return False
        return True

    def waitForBlockNumOnNode(self, blockNum, timeout=60):
        startTime=time.time()
        remainingTime=timeout
        while time.time()-startTime < timeout:
            if self.doesNodeHaveBlockNum(blockNum):
                return True
            sleepTime=3 if remainingTime > 3 else (3 - remainingTime)
            remainingTime -= sleepTime
            time.sleep(sleepTime)

        return False

    def waitForTransIdOnNode(self, transId, timeout=60):
        startTime=time.time()
        remainingTime=timeout
        while time.time()-startTime < timeout:
            if self.doesNodeHaveTransId(transId):
                return True
            sleepTime=3 if remainingTime > 3 else (3 - remainingTime)
            remainingTime -= sleepTime
            time.sleep(sleepTime)

        return False

    def getHeadBlockNum(self):
        num=None
        try:
            cmd="programs/eosc/eosc %s get info" % (self.endpointArgs)
            Utils.Debug and Utils.Print("cmd: %s" % (cmd))
            ret=subprocess.check_output(cmd.split(), stderr=subprocess.STDOUT).decode("utf-8")
            #MyPrint("eosc get info: ", ret)
            p=re.compile('\n\s+\"head_block_num\"\:\s+(\d+),\n', re.MULTILINE)
            m=p.search(ret)
            if m is None:
                Utils.Print("ERROR: Failed to parse head block num.")
                return -1

            s=m.group(1)
            num=int(s)

        except Exception as ex:
            msg=ex.output.decode("utf-8")
            Utils.Print("ERROR: Exception during block number retrieval. %s" % (msg))
            return -1

        return num

    def waitForNextBlock(self, timeout=60):
        startTime=time.time()
        remainingTime=timeout
        num=self.getHeadBlockNum()
        Utils.Debug and Utils.Print("Current block number: %s" % (num))
        
        while time.time()-startTime < timeout:
            nextNum=self.getHeadBlockNum()
            if nextNum > num:
                Utils.Debug and Utils.Print("Next block number: %s" % (nextNum))
                return True

            sleepTime=.5 if remainingTime > .5 else (.5 - remainingTime)
            remainingTime -= sleepTime
            time.sleep(sleepTime)

        return False

    # returns transaction id # for this transaction
    def transferFunds(self, source, destination, amount, memo="memo", force=False):
        transId=None
        p = re.compile('\n\s+\"transaction_id\":\s+\"(\w+)\",\n', re.MULTILINE)
        cmd="programs/eosc/eosc %s transfer %s %s %d" % (
            self.endpointArgs, source.name, destination.name, amount)
        cmdArgs=cmd.split()
        cmdArgs.append(memo)
        if force:
            cmdArgs.append("-f")
        Utils.Debug and Utils.Print("cmd: %s" % (cmdArgs))
        try:
            retStr=subprocess.check_output(cmdArgs, stderr=subprocess.STDOUT).decode("utf-8")
            #MyPrint("Ret: ", retStr)
            m=p.search(retStr)
            if m is None:
                Utils.Print("ERROR: transaction id parser failure")
                return None
            transId=m.group(1)
            #MyPrint("Transaction block num: %s" % (blockNum))
       
        except Exception as ex:
            msg=ex.output.decode("utf-8")
            Utils.Print("ERROR: Exception during funds transfer. %s" % (msg))
            return None

        return transId

    def validateSpreadFundsOnNode(self, adminAccount, accounts, expectedTotal):
        actualTotal=self.getAccountBalance(adminAccount.name)
        for account in accounts:
            fund = self.getAccountBalance(account.name)
            if fund != account.balance:
                Utils.Print("ERROR: validateSpreadFunds> Expected: %d, actual: %d for account %s" %
                        (account.balance, fund, account.name))
                return False
            actualTotal += fund

        if actualTotal != expectedTotal:
            Utils.Print("ERROR: validateSpreadFunds> Expected total: %d , actual: %d" % (
                expectedTotal, actualTotal))
            return False
    
        return True

    def getSystemBalance(self, adminAccount, accounts):
        balance=self.getAccountBalance(adminAccount.name)
        for account in accounts:
            balance += self.getAccountBalance(account.name)
        return balance

    def getAccountsByKey(self, key):
        try:
            cmd="programs/eosc/eosc %s get accounts %s" % (self.endpointArgs, key)
            Utils.Debug and Utils.Print("cmd: %s" % (cmd))
            retStr=subprocess.check_output(cmd.split()).decode("utf-8")
            #Utils.Print ("getAccountDetails> %s"% retStr)
            p=re.compile('\s+\"(\w+)\",?\n', re.MULTILINE)
            m=p.findall(retStr)
            if m is None:
                msg="Failed to account list by key."
                Utils.Print("ERROR: "+ msg)
                raise Exception(msg)

            return m
        except Exception as ex:
            msg=ex.output.decode("utf-8")
            Utils.Print("ERROR: Exception during accounts by key retrieval. %s" % (msg))
            raise

    def getServants(self, name):
        try:
            cmd="programs/eosc/eosc %s get servants %s" % (self.endpointArgs, name)
            Utils.Debug and Utils.Print("cmd: %s" % (cmd))
            retStr=subprocess.check_output(cmd.split()).decode("utf-8")
            #Utils.Print ("getAccountDetails> %s"% retStr)
            p=re.compile('\s+\"(\w+)\",?\n', re.MULTILINE)
            m=p.findall(retStr)
            if m is None:
                msg="Failed to account list by key."
                Utils.Print("ERROR: "+ msg)
                raise Exception(msg)

            return m
        except Exception as ex:
            msg=ex.output.decode("utf-8")
            Utils.Print("ERROR: Exception during servants retrieval. %s" % (msg))
            raise
        
    def getAccountDetailsByName(self, name):
        try:
            cmd="programs/eosc/eosc %s get account %s" % (self.endpointArgs, name)
            Utils.Debug and Utils.Print("cmd: %s" % (cmd))
            ret=subprocess.check_output(cmd.split()).decode("utf-8")
            #Utils.Print ("getAccountDetails> %s"% ret)
            p=re.compile('\n\s+\"eos_balance\"\:\s+\"(\d+.\d+)\s+EOS\",\n', re.MULTILINE)
            m=p.search(ret)
            if m is None:
                msg="Failed to parse account balance."
                Utils.Print("ERROR: "+ msg)
                raise Exception(msg)

            s=m.group(1)
            balance=float(s)
            balance *= 10000
            account=Account(name)
            account.balance=int(balance)
            return account

        except Exception as ex:
            msg=ex.output.decode("utf-8")
            Utils.Print("ERROR: Exception during account by name retrieval. %s" % (msg))
            raise

    def getAccountBalance(self, name):
        account=self.getAccountDetailsByName(name)
        if account is not None:
            return account.balance
        return None

    def getTransactionDetails(self, transId):
        try:
            cmd="programs/eosc/eosc %s get transaction %s" % (self.endpointArgs, transId)
            Utils.Debug and Utils.Print("cmd: %s" % (cmd))
            ret=subprocess.check_output(cmd.split()).decode("utf-8")
            #Utils.Print ("getAccountDetails> %s"% ret)
            p1=re.compile('\n\s+\"type\"\:\s+\"(\w+)\",\n', re.MULTILINE)
            p2=re.compile('\n\s+\"amount\"\:\s+(\d+),\n', re.MULTILINE)
            m1=p1.search(ret)
            if m1 is None:
                msg="Failed to parse transaction type."
                Utils.Print("ERROR: "+ msg)
                raise Exception(msg)

            tType=m1.group(1)

            m2=p2.search(ret)
            if m2 is None:
                msg="Failed to parse transaction amount."
                Utils.Print("ERROR: "+ msg)
                raise Exception(msg)

            amount=int(m2.group(1))

            transaction=Transaction(transId)
            transaction.tType=tType
            transaction.amount=amount
            return transaction

        except Exception as ex:
            msg=ex.output.decode("utf-8")
            Utils.Print("ERROR: Exception during account by transaction retrieval. %s" % (msg))
            raise
        
    def getTransactionsByAccount(self, name):
        try:
            cmd="programs/eosc/eosc %s get transactions %s" % (self.endpointArgs, name)
            Utils.Debug and Utils.Print("cmd: %s" % (cmd))
            retStr=subprocess.check_output(cmd.split()).decode("utf-8")
            #Utils.Print ("getAccountDetails> %s"% retStr)
            p=re.compile('\n\s+\"transaction_id\"\:\s+\"(\w+)\",\n', re.MULTILINE)
            m=p.findall(retStr)
            if m is None:
                msg="Failed to parse account list by key."
                Utils.Print("ERROR: "+ msg)
                raise Exception(msg)

            #Utils.Print ("m> %s"% m)
            transactions=[]
            for id in m:
                transaction=Transaction(id)
                transactions.append(transaction)
            return transactions
        except Exception as ex:
            msg=ex.output.decode("utf-8")
            Utils.Print("ERROR: Exception during transactions by account retrieval. %s" % (msg))
            raise

    def getAccountCodeHash(self, account):
        try:
            cmd="programs/eosc/eosc %s get code %s" % (self.endpointArgs, account)
            Utils.Debug and Utils.Print("cmd: %s" % (cmd))
            retStr=subprocess.check_output(cmd.split()).decode("utf-8")
            #Utils.Print ("getAccountDetails> %s"% retStr)
            p=re.compile('code\shash: (\w+)\n', re.MULTILINE)
            m=p.search(retStr)
            if m is None:
                msg="Failed to parse code hash."
                Utils.Print("ERROR: "+ msg)
                raise Exception(msg)

            return m.group(1)
        except Exception as ex:
            msg=ex.output.decode("utf-8")
            Utils.Print("ERROR: Exception during code hash retrieval. %s" % (msg))
            raise

    def publishContract(self, account, wastFile, abiFile, waitForTransBlock=False):
        transId=None
        p = re.compile('\n\s+\"transaction_id\":\s+\"(\w+)\",\n', re.MULTILINE)
        cmd="programs/eosc/eosc %s set contract %s %s %s" % (self.endpointArgs, account, wastFile, abiFile)
        Utils.Debug and Utils.Print("cmd: %s" % (cmd))
        try:
            retStr=subprocess.check_output(cmd.split()).decode("utf-8")
            Utils.Print ("getAccountDetails> %s"% retStr)
            m=p.search(retStr)
            if m is None:
                Utils.Print("ERROR: transaction id parser failure")
                return None
            transId=m.group(1)
        except Exception as ex:
            msg=ex.output.decode("utf-8")
            Utils.Print("ERROR: Exception during code hash retrieval. %s" % (msg))
            return None

        if waitForTransBlock and not self.waitForTransIdOnNode(transId):
            return None
        return transId

###########################################################################################

Wallet=namedtuple("Wallet", "name password host port")
class WalletMgr(object):
    __walletLogFile="test_walletd_output.log"
    __walletDataDir="test_wallet_0"

    # walletd [True|False] Will wallet me in a standalone process
    def __init__(self, walletd, eosdPort=8888, eosdHost="localhost", port=8899, host="localhost"):
        self.walletd=walletd
        self.eosdPort=eosdPort
        self.eosdHost=eosdHost
        self.port=port
        self.host=host
        self.wallets={}
        self.__walletPid=None
        self.endpointArgs="--host %s --port %d" % (self.eosdHost, self.eosdPort)
        if self.walletd:
            self.endpointArgs += " --wallet-host %s --wallet-port %d" % (self.host, self.port)

    def launch(self):
        if not self.walletd:
            Utils.Print("ERROR: Wallet Manager wasn't configured to launch walletd")
            return False
            
        cmd="programs/eos-walletd/eos-walletd --data-dir %s --http-server-address=%s:%d" % (
            WalletMgr.__walletDataDir, self.host, self.port)
        Utils.Print("cmd: %s" % (cmd))
        with open(WalletMgr.__walletLogFile, 'w') as sout, open(WalletMgr.__walletLogFile, 'w') as serr:
            popen=subprocess.Popen(cmd.split(), stdout=sout, stderr=serr)
            self.__walletPid=popen.pid

        # Give walletd time to warm up
        time.sleep(1)
        return True

    def create(self, name):
        wallet=self.wallets.get(name)
        if wallet is not None:
            Utils.Debug and Utils.Print("Wallet \"%s\" already exists. Returning same." % name)
            return wallet
        p = re.compile('\n\"(\w+)\"\n', re.MULTILINE)
        cmd="programs/eosc/eosc %s wallet create --name %s" % (self.endpointArgs, name)
        Utils.Debug and Utils.Print("cmd: %s" % (cmd))
        retStr=subprocess.check_output(cmd.split()).decode("utf-8")
        m=p.search(retStr)
        if m is None:
            Utils.Print("ERROR: wallet password parser failure")
            return None
        p=m.group(1)
        wallet=Wallet(name, p, self.host, self.port)
        self.wallets[name] = wallet
    
        return wallet

    #ciju
    def importKey(self, account, wallet):
        #Utils.Print("Private: ", key.private)
        warningMsg="This key is already imported into the wallet"
        cmd="programs/eosc/eosc %s wallet import --name %s %s" % (
            self.endpointArgs, wallet.name, account.ownerPrivateKey)
        Utils.Debug and Utils.Print("cmd: %s" % (cmd))
        try:
            retStr=subprocess.check_output(cmd.split(), stderr=subprocess.STDOUT).decode("utf-8")
        except Exception as ex:
            msg=ex.output.decode("utf-8")
            if warningMsg in msg:
                Utils.Print("WARNING: This key is already imported into the wallet.")
            else:
                Utils.Print("ERROR: Failed to import account owner key %s. %s" % (account.ownerPrivateKey, msg))
                return False

        if account.activePrivateKey is None:
            Utils.Print("WARNING: Active private key is not defined for account \"%s\"" % (account.name))
        else:
            cmd="programs/eosc/eosc %s wallet import --name %s %s" % (
                self.endpointArgs, wallet.name, account.activePrivateKey)
            Utils.Debug and Utils.Print("cmd: %s" % (cmd))
            try:
                retStr=subprocess.check_output(cmd.split(), stderr=subprocess.STDOUT).decode("utf-8")
            except Exception as ex:
                msg=ex.output.decode("utf-8")
                if warningMsg in msg:
                    Utils.Print("WARNING: This key is already imported into the wallet.")
                else:
                    Utils.Print("ERROR: Failed to import account active key %s. %s" %
                                (account.activePrivateKey, msg))
                    return False

        return True

    # def importKey(self, account, wallet):
    #     #Utils.Print("Private: ", key.private)
    #     cmd="programs/eosc/eosc %s wallet import --name %s %s" % (
    #         self.endpointArgs, wallet.name, account.ownerPrivateKey)
    #     Utils.Debug and Utils.Print("cmd: %s" % (cmd))
    #     if 0 != subprocess.call(cmd.split(), stdout=Utils.FNull):
    #         Utils.Print("ERROR: Failed to import account owner key %s." % (account.ownerPrivateKey))
    #         return False

    #     if account.activePrivateKey is None:
    #         Utils.Print("WARNING: Active private key is not defined for account \"%s\"" % (account.name))
    #     else:
    #         cmd="programs/eosc/eosc %s wallet import --name %s %s" % (
    #             self.endpointArgs, wallet.name, account.activePrivateKey)
    #         Utils.Debug and Utils.Print("cmd: %s" % (cmd))
    #         if 0 != subprocess.call(cmd.split(), stdout=Utils.FNull):
    #             Utils.Print("ERROR: Failed to import account active key %s." % (account.activePrivateKey))
    #             return False

    #     return True

    def lockWallet(self, wallet):
        cmd="programs/eosc/eosc %s wallet lock --name %s" % (self.endpointArgs, wallet.name)
        Utils.Debug and Utils.Print("cmd: %s" % (cmd))
        if 0 != subprocess.call(cmd.split(), stdout=Utils.FNull):
            Utils.Print("ERROR: Failed to lock wallet %s." % (wallet.name))
            return False

        return True

    def unlockWallet(self, wallet):
        cmd="programs/eosc/eosc %s wallet unlock --name %s" % (self.endpointArgs, wallet.name)
        Utils.Debug and Utils.Print("cmd: %s" % (cmd))
        popen=subprocess.Popen(cmd.split(), stdout=Utils.FNull, stdin=subprocess.PIPE)
        popen.communicate(input=wallet.password.encode("utf-8"))
        if 0 != popen.wait():
            Utils.Print("ERROR: Failed to unlock wallet %s." % (wallet.name))
            return False

        return True

    def lockAllWallets(self):
        cmd="programs/eosc/eosc %s wallet lock_all" % (self.endpointArgs)
        Utils.Debug and Utils.Print("cmd: %s" % (cmd))
        if 0 != subprocess.call(cmd.split(), stdout=Utils.FNull):
            Utils.Print("ERROR: Failed to lock all wallets.")
            return False

        return True

    def getOpenWallets(self):
        wallets=[]

        p = re.compile('\s+\"(\w+)\s\*\",?\n', re.MULTILINE)
        cmd="programs/eosc/eosc %s wallet list" % (self.endpointArgs)
        Utils.Debug and Utils.Print("cmd: %s" % (cmd))
        retStr=subprocess.check_output(cmd.split()).decode("utf-8")
        #Utils.Print("retStr: %s" % (retStr))
        m=p.findall(retStr)
        if m is None:
            Utils.Print("ERROR: wallet list parser failure")
            return None
        wallets=m

        return wallets

    def getKeys(self):
        keys=[]

        p = re.compile('\n\s+\"(\w+)\"\n', re.MULTILINE)
        cmd="programs/eosc/eosc %s wallet keys" % (self.endpointArgs)
        Utils.Debug and Utils.Print("cmd: %s" % (cmd))
        retStr=subprocess.check_output(cmd.split()).decode("utf-8")
        #Utils.Print("retStr: %s" % (retStr))
        m=p.findall(retStr)
        if m is None:
            Utils.Print("ERROR: wallet keys parser failure")
            return None
        keys=m

        return keys

    
    def dumpErrorDetails(self):
        Utils.Print("=================================================================")
        if self.__walletPid is not None:
            Utils.Print("Contents of %s:" % (Cluster.__walletLogFile))
            Utils.Print("=================================================================")
            shutil.copyfileobj(Cluster.__walletLogFile, sys.stdout)
    
    def killall(self):
        if self.__walletPid is not None:
            os.kill(self.__walletPid, signal.SIGKILL)
            
    def cleanup(self):
        dataDir=WalletMgr.__walletDataDir
        if os.path.isdir(dataDir) and os.path.exists(dataDir):
            shutil.rmtree(WalletMgr.__walletDataDir)



###########################################################################################
class Cluster(object):
    __chainSyncStrategies=Utils.getChainStrategies()
    __WalletName="MyWallet"
    __localHost="localhost"
    #__portStart=8888
    __lastTrans=None

    # init accounts
    initaAccount=Account("inita")
    initaAccount.ownerPrivateKey="5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3";
    initbAccount=Account("initb")
    initbAccount.ownerPrivateKey="5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3";

    # walletd [True|False] Is walletd running. If not load the wallet plugin
    def __init__(self, walletd=False, localCluster=True, host="localhost", port=8888, walletHost="localhost", walletPort=8899):
        self.accounts={}
        self.nodes={}
        self.localCluster=localCluster
        self.wallet=None
        self.walletd=walletd
        self.walletMgr=None
        self.host=host
        self.port=port
        self.walletHost=walletHost
        self.walletPort=walletPort
        self.walletEndpointArgs=""
        if self.walletd:
            self.walletEndpointArgs += " --wallet-host %s --wallet-port %d" % (self.walletHost, self.walletPort)
        #self.endpointArgs="--host %s --port %d" % (self.host, self.port)
        self.endpointArgs=""

    # ciju
    def setChainStrategy(chainSyncStrategy=Utils.SyncReplayTag):
        self.__chainSyncStrategy=self.__chainSyncStrategies.get(chainSyncStrategy)
        if self.__chainSyncStrategy is None:
            self.__chainSyncStrategy= __chainSyncStrategies.get("none")

    def setWalletMgr(walletMgr):
        self.walletMgr=walletMgr

    # launch local nodes and set self.nodes
    def launch(self, pnodes=1, total_nodes=1, topo="mesh", delay=1):
        if not self.localCluster:
            Utils.Print("WARNING: Cluster not local, not launching eosd.")
            return True
        
        if len(self.nodes) > 0:
            raise RuntimeError("Cluster already running.")

        cmd="programs/launcher/launcher -p %s -n %s -s %s -d %s" % (
            pnodes, total_nodes, topo, delay)
        cmdArr=cmd.split()
        if not self.walletd:
            cmdArr.append("--eosd")
            cmdArr.append("--plugin eosio::wallet_api_plugin")
        Utils.Print("cmd: ", cmdArr)
        if 0 != subprocess.call(cmdArr):
            Utils.Print("ERROR: Launcher failed to launch.")
            return False

        nodes=self.discoverLocalNodes(total_nodes)

        if total_nodes != len(nodes):
            Utils.Print("ERROR: Unable to validate eosd instances, expected: %d, actual: %d" %
                          (total_nodes, len(nodes)))
            return False

        self.nodes=nodes
        return True

    # manually set nodes, alternative to explicitly launch
    def setNodes(nodes):
        self.nodes=nodes

    # If a last transaction exists wait for it on root node, then collect its head block number.
    #  Wait on this block number on each cluster node
    def waitOnClusterSync(self, timeout=60):
        startTime=time.time()
        if self.nodes[0].alive is False:
            Utils.Print("ERROR: Root node is down.")
            return False;

        if self.__lastTrans is not None:
            if self.nodes[0].waitForTransIdOnNode(self.__lastTrans) is False:
                Utils.Print("ERROR: Failed to wait for last known transaction(%s) on root node." %
                            (self.__lastTrans))
                return False;

        targetHeadBlockNum=self.nodes[0].getHeadBlockNum() #get root nodes head block num
        Utils.Debug and Utils.Print("Head block number on root node: %d" % (targetHeadBlockNum))
        if targetHeadBlockNum == -1:
            return False

        currentTimeout=timeout-(time.time()-startTime)
        return self.waitOnClusterBlockNumSync(targetHeadBlockNum, currentTimeout)

    def waitOnClusterBlockNumSync(self, targetHeadBlockNum, timeout=60):
        startTime=time.time()
        remainingTime=timeout
        while time.time()-startTime < timeout:
            synced=True
            for node in self.nodes:
                if node.alive:
                    if node.doesNodeHaveBlockNum(targetHeadBlockNum) is False:
                        synced=False
                        break

            if synced is True:
                return True
            #Utils.Debug and Utils.Print("Brief pause to allow nodes to catch up.")
            sleepTime=3 if remainingTime > 3 else (3 - remainingTime)
            remainingTime -= sleepTime
            time.sleep(sleepTime)

        return False

    @staticmethod
    def createAccountKeys(count):
        accounts=[]
        p = re.compile('Private key: (.+)\nPublic key: (.+)\n', re.MULTILINE)
        for i in range(0, count):
            try:
                cmd="programs/eosc/eosc create key"
                Utils.Debug and Utils.Print("cmd: %s" % (cmd))
                keyStr=subprocess.check_output(cmd.split(), stderr=subprocess.STDOUT).decode("utf-8")
                m=p.match(keyStr)
                if m is None:
                    Utils.Print("ERROR: Owner key creation regex mismatch")
                    break
            
                ownerPrivate=m.group(1)
                ownerPublic=m.group(2)

                cmd="programs/eosc/eosc create key"
                Utils.Debug and Utils.Print("cmd: %s" % (cmd))
                keyStr=subprocess.check_output(cmd.split(), stderr=subprocess.STDOUT).decode("utf-8")
                m=p.match(keyStr)
                if m is None:
                    Utils.Print("ERROR: Owner key creation regex mismatch")
                    break
            
                activePrivate=m.group(1)
                activePublic=m.group(2)

                name=''.join(random.choice(string.ascii_lowercase) for _ in range(5))
                account=Account(name)
                account.ownerPrivateKey=ownerPrivate
                account.ownerPublicKey=ownerPublic
                account.activePrivateKey=activePrivate
                account.activePublicKey=activePublic
                accounts.append(account)

            except Exception as ex:
                msg=ex.output.decode("utf-8")
                Utils.Print("ERROR: Exception during key creation. %s" % (msg))
                break

        if count != len(accounts):
            Utils.Print("Account keys creation failed. Expected %d, actual: %d" % (count, len(accounts)))
            return None

        return accounts

    # create account keys and import into wallet. Wallet initialization needs to be done externally
    def populateWallet(self, accountsCount, wallet):
        if walletMgr is None:
            Utils.Print("ERROR: WalletMgr hasn't been initialized.")
            return False

        accounts=None
        if accountsCount > 0:
            Utils.Print ("Create account keys.")
            accounts = self.createAccountKeys(accountsCount)
            if accounts is None:
                Utils.Print("Account keys creation failed.")
                return False

        Print("Importing keys for account %s into wallet %s." % (initaAccount.name, wallet.name))
        if not walletMgr.importKey(Cluster.initaAccount, wallet):
            Utils.Print("ERROR: Failed to import key for account %s" % (initaAccount.name))
            return False

        Print("Importing keys for account %s into wallet %s." % (initbAccount.name, wallet.name))
        if not walletMgr.importKey(Cluster.initbAccount, wallet):
            Utils.Print("ERROR: Failed to import key for account %s" % (initbAccount.name))
            return False

        for account in accounts:
            Print("Importing keys for account %s into wallet %s." % (account.name, wallet.name))
            if not walletMgr.importKey(account, wallet):
                errorExit("Failed to import key for account %s" % (account.name))
                return False
        
    # def populateWallet(self, accountsCount):
    #     accounts=None
    #     if accountsCount > 0:
    #         Utils.Print ("Create account keys.")
    #         accounts = self.createAccountKeys(accountsCount)
    #         if accounts is None:
    #             Utils.Print("Account keys creation failed.")
    #             return False

    #     if self.createWallet(accounts) is False:
    #         return False

    #     return True

    # # Initialize wallet and import account keys
    # def createWallet_old(self, accounts):
    #     cmd="programs/eosc/eosc wallet create --name %s" % (self.__WalletName)
    #     Utils.Debug and Utils.Print("cmd: %s" % (cmd))
    #     if 0 != subprocess.call(cmd.split(), stdout=Utils.FNull):
    #         Utils.Print("ERROR: Failed to create wallet.")
    #         return False

    #     cmd="programs/eosc/eosc wallet import --name %s %s" % (
    #         self.__WalletName, Cluster.initaAccount.ownerPrivateKey)
    #     Utils.Debug and Utils.Print("cmd: %s" % (cmd))
    #     if 0 != subprocess.call(cmd.split(), stdout=Utils.FNull):
    #         Utils.Print("ERROR: Failed to import account inita key.")
    #         return False

    #     if accounts is None:
    #         return True

    #     self.accounts=accounts
    #     for account in self.accounts:
    #         #Utils.Print("Private: ", key.private)
    #         cmd="programs/eosc/eosc wallet import --name %s %s" % (self.__WalletName, account.ownerPrivateKey)
    #         Utils.Debug and Utils.Print("cmd: %s" % (cmd))
    #         if 0 != subprocess.call(cmd.split(), stdout=Utils.FNull):
    #             Utils.Print("ERROR: Failed to import account owner key.")
    #             return False

    #         cmd="programs/eosc/eosc wallet import --name %s %s" % (self.__WalletName, account.activePrivateKey)
    #         Utils.Debug and Utils.Print("cmd: %s" % (cmd))
    #         if 0 != subprocess.call(cmd.split(), stdout=Utils.FNull):
    #             Utils.Print("ERROR: Failed to import account active key.")
    #             return False

    #     return True

    def getNode(self, id=0):
        return self.nodes[0]
    
    # Spread funds across accounts with transactions spread through cluster nodes.
    #  Validate transactions are synchronized on root node
    def spreadFunds(self, amount=1):
        if len(self.accounts) == 0:
            return True

        count=len(self.accounts)
        transferAmount=(count*amount)+amount
        node=self.nodes[0]
        fromm=Cluster.initaAccount
        to=self.accounts[0]
        Utils.Print("Transfer %d units from account %s to %s on eos server port %d" % (
            transferAmount, fromm.name, to.name, node.port))
        transId=node.transferFunds(fromm, to, transferAmount)
        if transId is None:
            return False
        self.__lastTrans=transId
        Utils.Debug and Utils.Print("Funds transfered on transaction id %s." % (transId))
        self.accounts[0].balance += transferAmount

        nextEosIdx=-1
        for i in range(0, count):
            account=self.accounts[i]
            nextInstanceFound=False
            for n in range(0, count):
                #Utils.Print("nextEosIdx: %d, n: %d" % (nextEosIdx, n))
                nextEosIdx=(nextEosIdx + 1)%count
                if self.nodes[nextEosIdx].alive:
                    #Utils.Print("nextEosIdx: %d" % (nextEosIdx))
                    nextInstanceFound=True
                    break

            if nextInstanceFound is False:
                Utils.Print("ERROR: No active nodes found.")
                return False
            
            #Utils.Print("nextEosIdx: %d, count: %d" % (nextEosIdx, count))
            node=self.nodes[nextEosIdx]
            Utils.Debug and Utils.Print("Wait for trasaction id %s on node port %d" % (transId, node.port))
            if node.waitForTransIdOnNode(transId) is False:
                Utils.Print("ERROR: Selected node never received transaction id %s" % (transId))
                return False
        
            transferAmount -= amount
            fromm=account
            to=self.accounts[i+1] if i < (count-1) else Cluster.initaAccount
            Utils.Print("Transfer %d units from account %s to %s on eos server port %d." %
                    (transferAmount, fromm.name, to.name, node.port))

            transId=node.transferFunds(fromm, to, transferAmount)
            if transId is None:
                return False
            self.__lastTrans=transId
            Utils.Debug and Utils.Print("Funds transfered on block num %s." % (transId))

            self.accounts[i].balance -= transferAmount
            if i < (count-1):
                 self.accounts[i+1].balance += transferAmount

        # As an extra step wait for last transaction on the root node
        node=self.nodes[0]
        Utils.Debug and Utils.Print("Wait for trasaction id %s on node port %d" % (transId, node.port))
        if node.waitForTransIdOnNode(transId) is False:
            Utils.Print("ERROR: Selected node never received transaction id %s" % (transId))
            return False
    
        return True

    def validateSpreadFunds(self, expectedTotal):
        for node in self.nodes:
            if node.alive:
                Utils.Debug and Utils.Print("Validate funds on eosd server port %d." % (node.port))
                if node.validateSpreadFundsOnNode(Cluster.initaAccount, self.accounts, expectedTotal) is False:
                    Utils.Print("ERROR: Failed to validate funds on eos node port: %d" % (node.port))
                    return False

        return True

    def spreadFundsAndValidate(self, amount=1):
        Utils.Debug and Utils.Print("Get system balance.")
        initialFunds=self.nodes[0].getSystemBalance(Cluster.initaAccount, self.accounts)
        Utils.Debug and Utils.Print("Initial system balance: %d" % (initialFunds))

        if False == self.spreadFunds(amount):
            Utils.Print("ERROR: Failed to spread funds across nodes.")
            return False

        Utils.Print("Funds spread across all accounts")

        Utils.Print("Validate funds.")
        if False == self.validateSpreadFunds(initialFunds):
            Utils.Print("ERROR: Failed to validate funds transfer across nodes.")
            return False

        return True

    # create account, verify account and return transaction id
    def createAccountAndVerify(self, account, wallet):
        if len(self.nodes) == 0:
            Utils.Print("ERROR: No nodes initialized.")
            return None
        node=self.nodes[0]

        transId=node.createAccount(account, wallet)

        if transId is not None and node.verifyAccount(account):
            return transId
        return None

    # # Create account and return creation transactions
    # def createAccount(self, accountInfo):
    #     transId=None
    #     p = re.compile('\n\s+\"transaction_id\":\s+\"(\w+)\",\n', re.MULTILINE)
    #     cmd="programs/eosc/eosc create account %s %s %s %s" % (
    #         Cluster.initaAccount.name, accountInfo.name, accountInfo.ownerPublicKey,
    #         accountInfo.activePublicKey)
    #     Utils.Debug and Utils.Print("cmd: %s" % (cmd))
    #     try:
    #         retStr=subprocess.check_output(cmd.split()).decode("utf-8")
    #         # Utils.Print("Ret: ", retStr)
    #         m=p.search(retStr)
    #         if m is None:
    #             Utils.Print("ERROR: transaction id parser failure")
    #             return None
    #         transId=m.group(1)
    #         # Utils.Print("Transaction num: %s" % (transId))
       
    #     except Exception as ex:
    #         Utils.Print("ERROR: Exception during inita account creation.", ex)
    #         return None

    #     try :
    #         cmd="programs/eosc/eosc get account %s" % (accountInfo.name)
    #         Utils.Debug and Utils.Print("cmd: %s" % (cmd))
    #         ret=subprocess.check_output(cmd.split()).decode("utf-8")
    #         #Utils.Print ("ret:", ret)
    #         p=re.compile('\n\s+\"staked_balance\"\:\s+\"\d+.\d+\s+EOS\",\n', re.MULTILINE)
    #         m=p.search(ret)
    #         if m is None:
    #             Utils.Print("ERROR: Failed to validate account creation.", accountInfo.name)
    #             return None
    #     except Exception as e:
    #         Utils.Print("ERROR: Exception during account creation validation:", e)
    #         return None
    
    #     return transId

    # Populates list of EosInstanceInfo objects, matched to actual running instances
    def discoverLocalNodes(self, totalNodes):
        nodes=[]

        try:
            cmd="pgrep -a eosd"
            Utils.Debug and Utils.Print("cmd: %s" % (cmd))
            psOut=subprocess.check_output(cmd.split()).decode("utf-8")
            #Utils.Print("psOut: <%s>" % psOut)

            for i in range(0, totalNodes):
                pattern="[\n]?(\d+) (.* --data-dir tn_data_%02d)\n" % (i)
                m=re.search(pattern, psOut, re.MULTILINE)
                if m is None:
                    Utils.Print("ERROR: Failed to find eosd pid. Pattern %s" % (pattern))
                    break
                instance=Node(self.host, self.port + i, int(m.group(1)), m.group(2), True)
                instance.setWalletEndpointArgs(self.walletEndpointArgs)
                Utils.Debug and Utils.Print("Node:", instance)
                nodes.append(instance)
        except Exception as ex:
            Utils.Print("ERROR: Exception during Nodes creation:", ex)
            raise
        
        return nodes

    # Check state of running eosd process and update EosInstanceInfos
    #def updateEosInstanceInfos(eosInstanceInfos):
    def updateNodesStatus(self):
        for i in range(0, len(self.nodes)):
            node=self.nodes[i]
            running=True
            try:
                os.kill(node.pid, 0)
            except Exception as ex:
                running=False
            if running != node.alive:
                self.nodes[i].alive=running

    # Kills a percentange of Eos instances starting from the tail and update eosInstanceInfos state
    def killSomeEosInstances(self, killCount, killSignalStr=Utils.SigKillTag):
        killSignal=signal.SIGKILL
        if killSignalStr == Utils.SigTermTag:
            killSignal=signal.SIGTERM
        # killCount=int((percent/100.0)*len(self.nodes))
        Utils.Print("Kill %d Eosd instances with signal %s." % (killCount, killSignal))

        killedCount=0
        for node in reversed(self.nodes):
            try:
                os.kill(node.pid, killSignal)
            except Exception as ex:
                Utils.Print("ERROR: Failed to kill process pid %d." % (node.pid), ex)
                return False
            killedCount += 1
            if killedCount >= killCount:
                break

        time.sleep(1) # Give processes time to stand down
        return self.updateNodesStatus()

    def relaunchEosInstances(self):

        chainArg=self.__chainSyncStrategy.arg

        for i in range(0, len(self.nodes)):
            node=self.nodes[i]
            running=True
            try:
                os.kill(node.pid, 0) #check if instance is running
            except Exception as ex:
                running=False

            if running is False:
                dataDir="tn_data_%02d" % (i)
                dt = datetime.datetime.now()
                dateStr="%d_%02d_%02d_%02d_%02d_%02d" % (
                    dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second)
                stdoutFile="%s/stdout.%s.txt" % (dataDir, dateStr)
                stderrFile="%s/stderr.%s.txt" % (dataDir, dateStr)
                with open(stdoutFile, 'w') as sout, open(stderrFile, 'w') as serr:
                    cmd=node.cmd + ("" if chainArg is None else (" " + chainArg))
                    Utils.Print("cmd: %s" % (cmd))
                    popen=subprocess.Popen(cmd.split(), stdout=sout, stderr=serr)
                    self.nodes[i].pid=popen.pid

        return self.updateNodesStatus()

    def dumpErrorDetails(self):
        for i in range(0, len(self.nodes)):
            for f in ("config.ini", "stderr.txt"):
                Utils.Print("=================================================================")
                fileName="tn_data_%02d/%s" % (i, f)
                Utils.Print("Contents of %s:" % (fileName))
                with open(fileName, "r") as f:
                    shutil.copyfileobj(f, sys.stdout)
    
    def killall(self):
        cmd="programs/launcher/launcher -k 15"
        Utils.Debug and Utils.Print("cmd: %s" % (cmd))
        if 0 != subprocess.call(cmd.split(), stdout=Utils.FNull):
            Utils.Print("Launcher failed to shut down eos cluster.")

        for node in self.nodes:
            try:
                os.kill(node.pid, signal.SIGKILL)
            except Exception as ex:
                pass
            
    def waitForNextBlock(self, timeout=60):
        node=self.nodes[0]
        return node.waitForNextBlock()
    
    def cleanup(self):
        for f in glob.glob("tn_data_*"):
            shutil.rmtree(f)
            
    # Create accounts and validates that the last transaction is received on root node
    def createAccounts(self, wallet):
        if self.accounts is None:
            return True

        transId=None
        for account in self.accounts:
            Utils.Debug and Utils.Print("Create account %s." % (account.name))
            transId=self.createAccountAndVerify(account, wallet)
            if transId is None:
                Utils.Print("ERROR: Failed to create account %s." % (account.name))
                return False
            Utils.Debug and Utils.Print("Account %s created." % (account.name))

        if transId is not None:
            node=self.nodes[0]
            Utils.Debug and Utils.Print("Wait for transaction id %s on server port %d." % ( transId, node.port))
            if node.waitForTransIdOnNode(transId) is False:
                Utils.Print("ERROR: Failed waiting for transaction id %s on server port %d." % (
                    transId, node.port))
                return False

        return True
###########################################################################################
