/*
 * Test suite for TaskScheduler interface
 *
 * Copyright (C) 2008 Google (Roy Shea)
 * Copyright (C) 2018 Dmitry Timoshkov
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#define COBJMACROS

#include "corerror.h"

#include "initguid.h"
#include "mstask.h"
#include "wine/test.h"

static ITaskScheduler *test_task_scheduler;

static const WCHAR does_not_existW[] = {'\\','\\','d','o','e','s','_','n','o','t','_','e','x','i','s','t',0};

static void test_NewWorkItem(void)
{
    HRESULT hres;
    ITask *task;
    const WCHAR task_name[] = {'T', 'e', 's', 't', 'i', 'n', 'g', 0};
    GUID GUID_BAD;

    /* Initialize a GUID that will not be a recognized CLSID or a IID */
    CoCreateGuid(&GUID_BAD);

    /* Create TaskScheduler */
    hres = CoCreateInstance(&CLSID_CTaskScheduler, NULL, CLSCTX_INPROC_SERVER,
            &IID_ITaskScheduler, (void **) &test_task_scheduler);
    ok(hres == S_OK, "CTaskScheduler CoCreateInstance failed: %08x\n", hres);
    if (hres != S_OK)
    {
        skip("Failed to create task scheduler.  Skipping tests.\n");
        return;
    }

    /* Test basic task creation */
    hres = ITaskScheduler_NewWorkItem(test_task_scheduler, task_name,
            &CLSID_CTask, &IID_ITask, (IUnknown**)&task);
    ok(hres == S_OK, "NewNetworkItem failed: %08x\n", hres);
    if (hres == S_OK)
        ITask_Release(task);

    /* Task creation attempt using invalid work item class ID */
    hres = ITaskScheduler_NewWorkItem(test_task_scheduler, task_name,
            &GUID_BAD, &IID_ITask, (IUnknown**)&task);
    ok(hres == CLASS_E_CLASSNOTAVAILABLE,
            "Expected CLASS_E_CLASSNOTAVAILABLE: %08x\n", hres);

    /* Task creation attempt using invalid interface ID */
    hres = ITaskScheduler_NewWorkItem(test_task_scheduler, task_name,
            &CLSID_CTask, &GUID_BAD, (IUnknown**)&task);
    ok(hres == E_NOINTERFACE, "Expected E_NOINTERFACE: %08x\n", hres);

    /* Task creation attempt using invalid work item class and interface ID */
    hres = ITaskScheduler_NewWorkItem(test_task_scheduler, task_name,
            &GUID_BAD, &GUID_BAD, (IUnknown**)&task);
    ok(hres == CLASS_E_CLASSNOTAVAILABLE,
            "Expected CLASS_E_CLASSNOTAVAILABLE: %08x\n", hres);

    ITaskScheduler_Release(test_task_scheduler);
    return;
}

static void test_Activate(void)
{
    HRESULT hres;
    ITask *task = NULL;
    const WCHAR not_task_name[] =
            {'N', 'o', 'S', 'u', 'c', 'h', 'T', 'a', 's', 'k', 0};

    /* Create TaskScheduler */
    hres = CoCreateInstance(&CLSID_CTaskScheduler, NULL, CLSCTX_INPROC_SERVER,
            &IID_ITaskScheduler, (void **) &test_task_scheduler);
    ok(hres == S_OK, "CTaskScheduler CoCreateInstance failed: %08x\n", hres);
    if (hres != S_OK)
    {
        skip("Failed to create task scheduler.  Skipping tests.\n");
        return;
    }

    /* Attempt to activate a nonexistent task */
    hres = ITaskScheduler_Activate(test_task_scheduler, not_task_name,
            &IID_ITask, (IUnknown**)&task);
    ok(hres == COR_E_FILENOTFOUND, "Expected COR_E_FILENOTFOUND: %08x\n", hres);

    ITaskScheduler_Release(test_task_scheduler);
    return;
}

static void test_GetTargetComputer(void)
{
    HRESULT hres;
    WCHAR *oldname;

    /* Create TaskScheduler */
    hres = CoCreateInstance(&CLSID_CTaskScheduler, NULL, CLSCTX_INPROC_SERVER,
            &IID_ITaskScheduler, (void **) &test_task_scheduler);
    ok(hres == S_OK, "CTaskScheduler CoCreateInstance failed: %08x\n", hres);
    if (hres != S_OK)
    {
        skip("Failed to create task scheduler.\n");
        return;
    }

    if (0)
    {
        /* This crashes on w2k */
        hres = ITaskScheduler_GetTargetComputer(test_task_scheduler, NULL);
        ok(hres == E_INVALIDARG, "got 0x%x (expected E_INVALIDARG)\n", hres);
    }

    hres = ITaskScheduler_GetTargetComputer(test_task_scheduler, &oldname);
    ok((hres == S_OK) && oldname && oldname[0] == '\\' && oldname[1] == '\\' && oldname[2],
        "got 0x%x and %s (expected S_OK and an unc name)\n", hres, wine_dbgstr_w(oldname));

    CoTaskMemFree(oldname);

    ITaskScheduler_Release(test_task_scheduler);
    return;
}

static void test_SetTargetComputer(void)
{
    WCHAR buffer[MAX_COMPUTERNAME_LENGTH + 3];  /* extra space for two '\' and a zero */
    DWORD len = MAX_COMPUTERNAME_LENGTH + 1;    /* extra space for a zero */
    WCHAR *oldname = NULL;
    WCHAR *name = NULL;
    HRESULT hres;


    buffer[0] = '\\';
    buffer[1] = '\\';
    if (!GetComputerNameW(buffer + 2, &len))
        return;

    /* Create TaskScheduler */
    hres = CoCreateInstance(&CLSID_CTaskScheduler, NULL, CLSCTX_INPROC_SERVER,
            &IID_ITaskScheduler, (void **) &test_task_scheduler);
    ok(hres == S_OK, "CTaskScheduler CoCreateInstance failed: %08x\n", hres);
    if (hres != S_OK)
    {
        skip("Failed to create task scheduler.  Skipping tests.\n");
        return;
    }

    hres = ITaskScheduler_GetTargetComputer(test_task_scheduler, &oldname);
    ok(hres == S_OK, "got 0x%x and %s (expected S_OK)\n", hres, wine_dbgstr_w(oldname));

    /* NULL is an alias for the local computer */
    hres = ITaskScheduler_SetTargetComputer(test_task_scheduler, NULL);
    ok(hres == S_OK, "got 0x%x (expected S_OK)\n", hres);
    hres = ITaskScheduler_GetTargetComputer(test_task_scheduler, &name);
    ok((hres == S_OK && !lstrcmpiW(name, buffer)),
        "got 0x%x with %s (expected S_OK and %s)\n",
        hres, wine_dbgstr_w(name), wine_dbgstr_w(buffer));
    CoTaskMemFree(name);

    /* The name must be valid */
    hres = ITaskScheduler_SetTargetComputer(test_task_scheduler, does_not_existW);
    ok(hres == HRESULT_FROM_WIN32(ERROR_BAD_NETPATH), "got 0x%x (expected 0x80070035)\n", hres);
    /* the name of the target computer is unchanged */
    hres = ITaskScheduler_GetTargetComputer(test_task_scheduler, &name);
    ok((hres == S_OK && !lstrcmpiW(name, buffer)),
        "got 0x%x with %s (expected S_OK and %s)\n",
        hres, wine_dbgstr_w(name), wine_dbgstr_w(buffer));
    CoTaskMemFree(name);

    /* the two backslashes are optional */
    hres = ITaskScheduler_SetTargetComputer(test_task_scheduler, oldname + 2);
    if (hres == E_ACCESSDENIED)
    {
        skip("SetTargetComputer failed with E_ACCESSDENIED (needs admin rights)\n");
        goto done;
    }
    ok(hres == S_OK, "got 0x%x (expected S_OK)\n", hres);

    /* the case is ignored */
    CharUpperW(buffer);
    hres = ITaskScheduler_SetTargetComputer(test_task_scheduler, buffer);
    ok(hres == S_OK, "got 0x%x (expected S_OK)\n", hres);
    CharLowerW(buffer);
    hres = ITaskScheduler_SetTargetComputer(test_task_scheduler, buffer);
    ok(hres == S_OK, "got 0x%x (expected S_OK)\n", hres);

    /* cleanup */
    hres = ITaskScheduler_SetTargetComputer(test_task_scheduler, oldname);
    ok(hres == S_OK, "got 0x%x (expected S_OK)\n", hres);

done:
    CoTaskMemFree(oldname);
    ITaskScheduler_Release(test_task_scheduler);
    return;
}

static void test_Enum(void)
{
    ITaskScheduler *scheduler;
    IEnumWorkItems *tasks;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_CTaskScheduler, NULL, CLSCTX_INPROC_SERVER,
            &IID_ITaskScheduler, (void **)&scheduler);
    ok(hr == S_OK, "got 0x%08x\n", hr);

if (0) { /* crashes on win2k */
    hr = ITaskScheduler_Enum(scheduler, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);
}

    hr = ITaskScheduler_Enum(scheduler, &tasks);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    IEnumWorkItems_Release(tasks);

    ITaskScheduler_Release(scheduler);
}

static BOOL file_exists(const WCHAR *name)
{
    return GetFileAttributesW(name) != INVALID_FILE_ATTRIBUTES;
}

static void test_save_task_curfile(ITask *task)
{
    HRESULT hr;
    IPersistFile *pfile;
    WCHAR *curfile;

    hr = ITask_QueryInterface(task, &IID_IPersistFile, (void **)&pfile);
    ok(hr == S_OK, "QueryInterface error %#x\n", hr);

    curfile = NULL;
    hr = IPersistFile_GetCurFile(pfile, &curfile);
    ok(hr == S_OK, "GetCurFile error %#x\n", hr);
    ok(curfile && curfile[0] , "curfile should not be NULL\n");

    ok(file_exists(curfile), "curfile should exist\n");

    hr = IPersistFile_Save(pfile, curfile, FALSE);
    ok(hr == HRESULT_FROM_WIN32(ERROR_FILE_EXISTS), "wrong error %#x\n", hr);

    hr = IPersistFile_Save(pfile, curfile, TRUE);
    ok(hr == HRESULT_FROM_WIN32(ERROR_FILE_EXISTS), "wrong error %#x\n", hr);

    hr = IPersistFile_Save(pfile, NULL, FALSE);
    ok(hr == S_OK, "Save error %#x\n", hr);

    hr = IPersistFile_Save(pfile, NULL, TRUE);
    ok(hr == S_OK, "Save error %#x\n", hr);

    curfile = NULL;
    hr = IPersistFile_GetCurFile(pfile, &curfile);
    ok(hr == S_OK, "GetCurFile error %#x\n", hr);
    ok(curfile && curfile[0] , "curfile should not be NULL\n");

    IPersistFile_Release(pfile);
}

static WCHAR *get_task_curfile(ITask *task)
{
    HRESULT hr;
    IPersistFile *pfile;
    WCHAR *curfile;
    CLSID clsid;

    hr = ITask_QueryInterface(task, &IID_IPersistFile, (void **)&pfile);
    ok(hr == S_OK, "QueryInterface error %#x\n", hr);
    curfile = NULL;
    hr = IPersistFile_GetCurFile(pfile, &curfile);
    ok(hr == S_OK, "GetCurFile error %#x\n", hr);
    ok(curfile && curfile[0] , "curfile should not be NULL\n");

    if (0) /* crashes under Windows */
        hr = IPersistFile_GetClassID(pfile, NULL);

    hr = IPersistFile_GetClassID(pfile, &clsid);
    ok(hr == S_OK, "GetClassID error %#x\n", hr);
    ok(IsEqualCLSID(&clsid, &CLSID_CTask), "got %s\n", wine_dbgstr_guid(&clsid));

    IPersistFile_Release(pfile);

    return curfile;
}

static void test_task_storage(void)
{
    static const WCHAR Task1[] = { 'w','i','n','e','t','a','s','k',0 };
    static const WCHAR Task1_job[] = { '\\','T','a','s','k','s','\\','w','i','n','e','t','a','s','k','.','j','o','b',0 };
    WCHAR task_full_name[MAX_PATH];
    HRESULT hr;
    ITaskScheduler *scheduler;
    ITask *task, *task2;
    WCHAR *curfile, *curfile2;

    GetWindowsDirectoryW(task_full_name, MAX_PATH);
    lstrcatW(task_full_name, Task1_job);

    hr = CoCreateInstance(&CLSID_CTaskScheduler, NULL, CLSCTX_INPROC_SERVER, &IID_ITaskScheduler, (void **)&scheduler);
    if (hr != S_OK)
    {
        win_skip("CoCreateInstance(CLSID_CTaskScheduler) error %#x\n", hr);
        return;
    }

    hr = ITaskScheduler_NewWorkItem(scheduler, Task1, &CLSID_CTask, &IID_ITask, (IUnknown **)&task);
    ok(hr == S_OK, "NewWorkItem error %#x\n", hr);

    curfile = get_task_curfile(task);
    ok(!file_exists(curfile), "curfile should not exist\n");
    ok(!lstrcmpW(curfile, task_full_name), "name is wrong %s\n", wine_dbgstr_w(curfile));

    hr = ITaskScheduler_NewWorkItem(scheduler, Task1, &CLSID_CTask, &IID_ITask, (IUnknown **)&task2);
    ok(hr == S_OK, "NewWorkItem error %#x\n", hr);
    ok(task2 != task, "tasks should not be equal\n");

    curfile2 = get_task_curfile(task);
    ok(!file_exists(curfile2), "curfile2 should not exist\n");
    ok(!lstrcmpW(curfile2, task_full_name), "name is wrong %s\n", wine_dbgstr_w(curfile2));

    CoTaskMemFree(curfile);
    CoTaskMemFree(curfile2);

    task2 = (ITask *)0xdeadbeef;
    hr = ITaskScheduler_Activate(scheduler, Task1, &IID_ITask, (IUnknown **)&task2);
    ok(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), "wrong error %#x\n", hr);
    ok(task2 == (ITask *)0xdeadbeef, "task should not be set to NULL\n");

    hr = ITaskScheduler_AddWorkItem(scheduler, Task1, (IScheduledWorkItem *)task);
    ok(hr == S_OK, "AddWorkItem error %#x\n", hr);

    curfile = get_task_curfile(task);
    ok(file_exists(curfile), "curfile should exist\n");

    hr = ITaskScheduler_AddWorkItem(scheduler, Task1, (IScheduledWorkItem *)task);
    ok(hr == HRESULT_FROM_WIN32(ERROR_FILE_EXISTS), "wrong error %#x\n", hr);

    curfile = get_task_curfile(task);
    ok(file_exists(curfile), "curfile should exist\n");

    ITask_Release(task);

    task = NULL;
    hr = ITaskScheduler_Activate(scheduler, Task1, &IID_ITask, (IUnknown **)&task);
    ok(hr == S_OK, "Activate error %#x\n", hr);
    ok(task != NULL, "task should not be set to NULL\n");

    curfile2 = get_task_curfile(task);
    ok(file_exists(curfile2), "curfile2 should exist\n");
    ok(!lstrcmpW(curfile2, task_full_name), "name is wrong %s\n", wine_dbgstr_w(curfile2));

    CoTaskMemFree(curfile2);

    test_save_task_curfile(task);

    DeleteFileW(curfile);
    CoTaskMemFree(curfile);

    ITask_Release(task);
    ITaskScheduler_Release(scheduler);
}

START_TEST(task_scheduler)
{
    CoInitialize(NULL);

    test_task_storage();
    test_NewWorkItem();
    test_Activate();
    test_GetTargetComputer();
    test_SetTargetComputer();
    test_Enum();
    CoUninitialize();
}
