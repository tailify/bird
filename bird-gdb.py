class BIRDPrinter:
    def __init__(self, val):
        self.val = val

    @classmethod
    def lookup(cls, val):
#        print(cls.typeCode, cls.typeTag, val.type.code, val.type.tag)
        if val.type.code != cls.typeCode:
            return None
        if val.type.tag != cls.typeTag:
            return None

        return cls(val)


class BIRDFValPrinter(BIRDPrinter):
    "Print BIRD\s struct f_val"
    typeCode = gdb.TYPE_CODE_STRUCT
    typeTag = "f_val"

    codemap = {
            "T_INT": "i",
            "T_BOOL": "i",
            "T_PAIR": "i",
            "T_QUAD": "i",
            "T_ENUM_RTS": "i",
            "T_ENUM_BGP_ORIGIN": "i",
            "T_ENUM_SCOPE": "i",
            "T_ENUM_RTC": "i",
            "T_ENUM_RTD": "i",
            "T_ENUM_ROA": "i",
            "T_ENUM_NETTYPE": "i",
            "T_ENUM_RA_PREFERENCE": "i",
            "T_IP": "ip",
            "T_NET": "net",
            "T_STRING": "s",
            "T_PATH_MASK": "path_mask",
            "T_PATH": "ad",
            "T_CLIST": "ad",
            "T_EC": "ec",
            "T_ECLIST": "ad",
            "T_LC": "lc",
            "T_LCLIST": "ad",
            "T_RD": "ec",
            "T_PATH_MASK_ITEM": "pmi",
            "T_SET": "t",
            "T_PREFIX_SET": "ti",
            }

    def to_string(self):
        code = self.val['type']
        if code.type.code != gdb.TYPE_CODE_ENUM or code.type.tag != "f_type":
            raise Exception("Strange 'type' element in f_val")

        if str(code) == "T_VOID":
            return "T_VOID"
        else:
            return "(%(c)s) %(v)s" % { "c": code, "v": self.val['val'][self.codemap[str(code)]] }

    def display_hint(self):
        return "map"

class BIRDFValStackPrinter(BIRDPrinter):
    "Print BIRD's struct f_val_stack"
    typeCode = gdb.TYPE_CODE_STRUCT
    typeTag = "f_val_stack"

    def to_string(self):
        cnt = self.val['cnt']
        return ("Value stack (%(cnt)d):\n\t" % { "cnt": cnt }) + \
                "\n\t".join([ (".val[%(n) 3d] = " % { "n": n}) + str(self.val['val'][n]) for n in range(cnt-1, -1, -1) ])

    def display_hint(self):
        return "map"

class BIRDFInstPrinter(BIRDPrinter):
    "Print BIRD's struct f_inst"
    typeCode = gdb.TYPE_CODE_STRUCT
    typeTag = "f_inst"

    def to_string(self):
        code = self.val['fi_code']
        if str(code) == "FI_NOP":
            return str(code) + ": " + str(self.val.cast(gdb.lookup_type("const char [%(siz)d]" % { "siz": self.val.type.sizeof })))
        return "%(code)s:\t%(lineno) 6dL\t%(size)6dS\tnext = %(next)s: .i_%(code)s = %(union)s" % {
                "code": str(code),
                "lineno": self.val['lineno'],
                "size": self.val['size'],
                "next": str(self.val['next']),
                "union": str(self.val['i_' + str(code)])
                }

# def children(self): # children iterator
    def display_hint(self):
        return "map"

class BIRDFLineItemPrinter(BIRDPrinter):
    "Print BIRD's struct f_line_item"
    typeCode = gdb.TYPE_CODE_STRUCT
    typeTag = "f_line_item"

    def to_string(self):
        code = self.val['fi_code']
        if str(code) == "FI_NOP":
            return str(code) + ": " + str(self.val.cast(gdb.lookup_type("const char [%(siz)d]" % { "siz": self.val.type.sizeof })))
        return "%(code)s:\t%(lineno) 6dL\t%(flags)2dF: .i_%(code)s = %(union)s" % {
                "code": str(code),
                "lineno": self.val['lineno'],
                "flags": self.val['flags'],
                "union": str(self.val['i_' + str(code)])
                }

class BIRDFLinePrinter(BIRDPrinter):
    "Print BIRD's struct f_line"
    typeCode = gdb.TYPE_CODE_STRUCT
    typeTag = "f_line"

    def to_string(self):
        cnt = self.val['len']
        return ("FLine (%(cnt)d, args=%(args)d): " % { "cnt": cnt, "args" : self.val['args'] } + \
                ", ".join([
                    ".items[%(n) 3d] = %(code)s" % {
                        "n": n,
                        "code": str(self.val['items'][n]['fi_code']),
                    } if n % 8 == 0 else str(self.val['items'][n]['fi_code']) for n in range(cnt)]))
        

class BIRDFExecStackPrinter(BIRDPrinter):
    "Print BIRD's struct f_exec_stack"
    typeCode = gdb.TYPE_CODE_STRUCT
    typeTag = "f_exec_stack"

    def to_string(self):
        cnt = self.val['cnt']
        return ("Exec stack (%(cnt)d):\n\t" % { "cnt": cnt }) + \
                "\n\t".join([ ".item[%(n) 3d] = %(retflag)d V%(ventry) 3d P%(pos) 4d %(line)s" % {
                    "retflag": self.val['item'][n]['emask'],
                    "ventry": self.val['item'][n]['ventry'],
                    "pos": self.val['item'][n]['pos'],
                    "line": str(self.val['item'][n]['line'].dereference()),
                    "n": n
                        } for n in range(cnt-1, -1, -1) ])

class BIRDWQStateLogPrinter(BIRDPrinter):
    "Print BIRD's worker queue struct worker_queue_state"
    typeCode = gdb.TYPE_CODE_STRUCT
    typeTag = "worker_queue_state"

    def queueval(self):
        return "%(worker) 4d %(what)s: pending %(pending)s, running %(running)s, stop %(stop)s" % {
                "worker": self.val['worker_id'],
                "what": str(self.val['what']),
                "pending": str(self.val['queue']['pending']),
                "running": str(self.val['queue']['running']),
                "stop": str(self.val['queue']['stop']),
                }

    def semval(self):
        semname = None
        semaddr = self.val["sem"]
        wq = gdb.lookup_symbol("wq")[0].value().dereference()
        for s in [ "waiting", "stopped", "yield", "available" ]:
            if semaddr == wq[s].address:
                if semname is not None:
                    raise Exception("Two matching semaphores!")

                semname = s

        if semname is None:
            raise Exception("No matching semaphore!")

        return "%(worker) 4d %(what)s %(semname)s" % {
                "worker": self.val['worker_id'],
                "what": str(self.val["what"]),
                "semname": semname
                }

    def domval(self):
        return "%(worker) 4d %(what)s: %(task)s on %(domain)s: RP=%(rdtasks_n)u WP=%(wrtasks_n)u RS=%(rdsem_n)u WS=%(wrsem_n)u RL=%(rdlocked)u PREP=%(prepended)u" % {
                "worker": self.val['worker_id'],
                "what": str(self.val["what"]),
                "task": str(self.val["domain"]["task"]),
                "domain": str(self.val["domain"]["domain"]),
                "rdtasks_n": self.val["domain"]["rdtasks_n"],
                "wrtasks_n": self.val["domain"]["wrtasks_n"],
                "rdsem_n": self.val["domain"]["rdsem_n"],
                "wrsem_n": self.val["domain"]["wrsem_n"],
                "rdlocked": self.val["domain"]["rdlocked"],
                "prepended": self.val["domain"]["prepended"],
                }

    def nothing(self):
        return "nothing"

    def to_string(self):
        return {
                "WQS_NOTHING": self.nothing,
                "WQS_LOCK": self.queueval,
                "WQS_UNLOCK": self.queueval,
                "WQS_YIELD": self.queueval,
                "WQS_CONTINUE": self.queueval,
                "WQS_SEM_POST": self.semval,
                "WQS_SEM_WAIT_REQUEST": self.semval,
                "WQS_SEM_WAIT_SUCCESS": self.semval,
                "WQS_SEM_TRYWAIT_SUCCESS": self.semval,
                "WQS_SEM_TRYWAIT_BLOCKED": self.semval,
                "WQS_DOMAIN_WRLOCK_REQUEST": self.domval,
                "WQS_DOMAIN_RDLOCK_REQUEST": self.domval,
                "WQS_DOMAIN_WRLOCK_SUCCESS": self.domval,
                "WQS_DOMAIN_RDLOCK_SUCCESS": self.domval,
                "WQS_DOMAIN_WRLOCK_BLOCKED": self.domval,
                "WQS_DOMAIN_RDLOCK_BLOCKED": self.domval,
                "WQS_DOMAIN_RDUNLOCK_REQUEST": self.domval,
                "WQS_DOMAIN_WRUNLOCK_REQUEST": self.domval,
                "WQS_DOMAIN_RDUNLOCK_DONE": self.domval,
                "WQS_DOMAIN_WRUNLOCK_DONE": self.domval,
                }[str(self.val['what'])]()

def register_printers(objfile):
    objfile.pretty_printers.append(BIRDFInstPrinter.lookup)
    objfile.pretty_printers.append(BIRDFValPrinter.lookup)
    objfile.pretty_printers.append(BIRDFValStackPrinter.lookup)
    objfile.pretty_printers.append(BIRDFLineItemPrinter.lookup)
    objfile.pretty_printers.append(BIRDFLinePrinter.lookup)
    objfile.pretty_printers.append(BIRDFExecStackPrinter.lookup)
    objfile.pretty_printers.append(BIRDWQStateLogPrinter.lookup)

register_printers(gdb.current_objfile())

class BIRDCommand(gdb.Command):
    def __init__(self):
        super().__init__("bird", gdb.COMMAND_NONE, gdb.COMPLETE_COMMAND, True)

    def invoke(self, argument, from_tty):
        pass

class BIRDWQCommand(gdb.Command):
    def __init__(self):
        super().__init__("bird wq", gdb.COMMAND_NONE, gdb.COMPLETE_COMMAND, True)

    def invoke(self, argument, from_tty):
        pass

class BIRDWQStateLogCommand(gdb.Command):
    "Show worker queue last state log"
    def __init__(self):
        super().__init__("bird wq statelog", gdb.COMMAND_DATA, gdb.COMPLETE_EXPRESSION)

    def invoke(self, argument, from_tty):
        args = gdb.string_to_argv(argument)
        if len(args) > 1:
            raise Exception("Too many args")

        total = gdb.parse_and_eval(args[0]) if len(args) >= 1 else 10
        wq = gdb.lookup_symbol("wq")[0].value().dereference()
        statelog_size = gdb.lookup_symbol("STATELOG_SIZE")[0].value()
        statelog_pos = wq["statelog_pos"]

        print("   idx  WID command")

        for i in range(total):
            idx = (statelog_pos + statelog_size - i - 1) % statelog_size
            print("%(idx) 5d" % { "idx": idx }, wq["statelog"][idx])

BIRDCommand()
BIRDWQCommand()
BIRDWQStateLogCommand()

print("BIRD pretty printers loaded OK.")