#!/usr/bin/env python3
"""Local-only resumable progress, never a compatibility or playability estimate."""
import argparse
from collections import Counter
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]
GATES = {
    "correct_iso_validated": 10, "generated_game_code_verified": 10,
    "required_converted_runtime_assets_verified": 10,
    "full_game_arm64_compiled_and_linked": 20, "metal_renderer_integrated_and_tested": 15,
    "ios_audio_controller_save_import_tested": 10, "installable_ipa_packaged": 10,
    "real_iphone_installed_and_launched": 10, "device_game_menu_audio_input": 5,
}
STOP_REASONS = {"PHASE_COMPLETE", "FIRST_FATAL_BLOCKER", "AUTHORIZATION_REQUIRED",
                "USAGE_LIMIT_REPORTED", "SESSION_INTERRUPTED_OR_UNKNOWN", "STILL_WORKING"}


def delivery_score(gates):
    assert sum(GATES.values()) == 100
    return sum(weight for name, weight in GATES.items() if gates.get(name, {}).get("status") == "PASS")


def atomic_json(path, value):
    part = path.with_suffix(path.suffix + ".part")
    part.write_text(json.dumps(value, indent=2) + "\n")
    part.replace(path)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--session-start", help="Explicit UTC start for a newly resumed session; otherwise retain current session")
    p.add_argument("--milestone", required=True)
    p.add_argument("--blocker", default="AUDIT_IN_PROGRESS")
    p.add_argument("--next-action", required=True)
    p.add_argument("--stop-reason", choices=sorted(STOP_REASONS), default="STILL_WORKING")
    p.add_argument("--test-results", type=Path)
    p.add_argument("--real-behaviour", action="append", default=[])
    p.add_argument("--change", action="append", default=[])
    p.add_argument("--classification", action="append", default=[])
    args = p.parse_args()
    path = ROOT / "RAC1_IOS_PROGRESS.json"
    state = json.loads(path.read_text()) if path.exists() else {}
    now = datetime.now(timezone.utc)
    start = datetime.fromisoformat(args.session_start or state.get("session_start", now.isoformat()))
    elapsed = max(0, int((now-start).total_seconds()))
    state.update(schema_version=1, timestamp=now.isoformat(), session_start=start.isoformat(),
                 session_elapsed_seconds=elapsed, stop_reason=args.stop_reason,
                 milestone=args.milestone, first_fatal_blocker=args.blocker,
                 next_technical_action=args.next_action, workspace=str(ROOT),
                 branch="ios/arm64-metal-port", source_commit="78ffc48b63a082bcd6a57f929a367fb1699eb2b1",
                 checkout_status="PINNED_MATERIALIZED_TREE_WITH_LOCAL_CHANGES_NO_LOCAL_HEAD",
                 quota_detection="NOT_EXPOSED_NO_QUOTA_CLAIM",
                 elapsed_time_basis="UTC_WALL_CLOCK_NOT_ACCOUNT_ACTIVE_USAGE",
                 checkpoint_at_4h45_required=elapsed >= 17100,
                 privacy="LOCAL_ONLY_NEVER_UPLOAD_CHECKPOINT_OR_GAME_DATA")
    git_commit=subprocess.run(["git","rev-parse","--verify","HEAD"],cwd=ROOT,capture_output=True,text=True)
    if git_commit.returncode==0:
        branch=subprocess.run(["git","branch","--show-current"],cwd=ROOT,capture_output=True,text=True,check=True).stdout.strip()
        state.update(source_commit=git_commit.stdout.strip(),branch=branch,
                     checkout_status="VERIFIED_SHALLOW_GIT_BASELINE_WITH_LOCAL_WORKING_CHANGES")
        patch=ROOT/"build/preserved-materialization.patch"
        if patch.exists():
            original=patch.read_bytes()
            current=subprocess.check_output(["git","diff","--binary","HEAD"],cwd=ROOT)
            state["preserved_worktree_patch"]={"path":str(patch),
                "sha256":hashlib.sha256(original).hexdigest(),
                "matches_current_tracked_delta":original==current,
                "scope":"Preserved pre-existing materialization differences; private reproducibility evidence"}
    gates = state.setdefault("delivery_gates", {n: {"weight": w, "status": "UNTESTED", "evidence": []} for n,w in GATES.items()})
    audit = ROOT / "build/private-setup-audit.json"
    if audit.exists():
        report = json.loads(audit.read_text())
        state["iso"] = report.get("iso", {})
        state["setup"] = report.get("setup", {})
        for name,key in (("correct_iso_validated","iso_validated"),("generated_game_code_verified","generated_verified")):
            gates[name].update(status="PASS" if report.get(key) else "FAIL", evidence=[str(audit)])
        gates["required_converted_runtime_assets_verified"].update(status="PASS" if report.get("converted_assets_verified") else "BLOCKED", evidence=[str(audit)])
    state["ipa_readiness"] = {"points": delivery_score(gates), "denominator": 100,
                              "meaning": "Fixed weighted delivery milestone score, NOT playability probability"}
    state.setdefault("engine_game_coverage", {"overall": "NOT MEASURABLE YET", "reason": "No discovered reachable-function/observed-hardware/gameplay denominator established",
        "furthest_verified_rac1_iphone_checkpoint": "NONE", "device_gameplay_tested": False})
    state.setdefault("tests", [])
    if args.test_results:
        state["tests"] = json.loads(args.test_results.read_text())
    # These denominators are deliberately scoped, not a percentage of the engine/game.
    scoped = []
    for test in state["tests"]:
        if test.get("name") == "selected_original_game_math_oracle" and test.get("status") == "PASS":
            counts = test["counts"]
            scoped.append({"metric": "selected_original_math_functions_passing",
                           "passed": counts["selected_functions"], "denominator": counts["selected_functions"],
                           "scope": "Preselected Intel/reference-interpreter comparison; NOT all reachable game functions"})
            scoped.append({"metric": "selected_original_math_comparisons_passing",
                           "passed": counts["comparisons"] - counts["failed"], "denominator": counts["comparisons"],
                           "scope": "Finite deterministic test inputs only; NOT ARM64 runtime or gameplay"})
    compile_receipt = ROOT / "build-private-arm64-generated/compilation-receipt.json"
    if compile_receipt.exists():
        compilation = json.loads(compile_receipt.read_text())
        state["arm64_compilation"] = {key: compilation.get(key) for key in
            ("compiled_units", "expected_units", "status", "experimental_investigation",
             "production_fidelity_verified", "full_game_linked", "ipa_created")}
        state["arm64_compilation"]["evidence"] = str(compile_receipt)
    mixed_receipt=ROOT/"build-private-arm64-O0-probes/compilation-receipt.json"
    if mixed_receipt.exists():
        mixed=json.loads(mixed_receipt.read_text())
        state["generated_source_compatibility"]={key:mixed.get(key) for key in
            ("covered_units","expected_units","status","scope","production_fidelity_verified","full_game_linked","ipa_created")}
        state["generated_source_compatibility"].update(evidence=str(mixed_receipt),
            optimization_counts=dict(Counter(unit["optimization"] for unit in mixed.get("covered",{}).values())))
    partial_receipt=ROOT/"build/private-generated-relocatable-link.json"
    if partial_receipt.exists():
        partial=json.loads(partial_receipt.read_text())
        state["generated_partial_link"]={key:partial.get(key) for key in
            ("exit_code","input_objects","link_kind","macho_type","unresolved_symbol_count",
             "object_sha256","production_fidelity_verified","application_linked","ipa_created")}
        state["generated_partial_link"]["evidence"]=str(partial_receipt)
    reasons={"full_game_arm64_compiled_and_linked":"Strict production FP toolchain gate blocked; native game/host/backend not linked as an executable",
        "metal_renderer_integrated_and_tested":"Metal components only; full scene/GS renderer and image validation absent",
        "ios_audio_controller_save_import_tested":"Complete UIKit game/audio/save/import lifecycle not integrated or tested",
        "installable_ipa_packaged":"No game executable/app bundle; partial MH_OBJECT is NOT an application"}
    for name,reason in reasons.items():
        if gates[name]["status"]!="PASS": gates[name].update(status="BLOCKED",reason=reason)
    state["engine_game_coverage"]["scoped_metrics"] = scoped
    if args.real_behaviour: state["real_behaviour_advanced"] = args.real_behaviour
    if args.change: state["changes"] = args.change
    state.setdefault("classification", ["GAME_SPECIFIC", "SHARED_DIAGNOSTICS", "SHARED_TESTING"])
    if args.classification: state["classification"]=args.classification
    state["classification_scope"]="REVIEW_CANDIDATES_ONLY_NO_SHARED_ADOPTION"
    state.update(shared_core_impact="NO_ADOPTION_NO_ENGINE_MODIFICATION", other_engines_needing_review=[])
    atomic_json(path, state)
    text = ["# RAC1 SESSION STATUS", "", f"Timestamp: {state['timestamp']}",
            f"Stop reason: {args.stop_reason}", f"Session elapsed: {elapsed//3600}h {(elapsed%3600)//60}m {elapsed%60}s",
            f"Source: {state['branch']} @ {state['source_commit']}",
            "Local worktree changes are preserved; original remote baseline remains intact.", "",
            f"Milestone: {args.milestone}", f"ISO/setup: {json.dumps(state.get('iso', {}))}",
            f"Setup stages: {json.dumps(state.get('setup', {}))}", "",
            f"IPA build readiness: {delivery_score(gates)}/100 fixed delivery points (not probability).", ""]
    text += [f"- {name}: {row['status']} ({GATES[name]} points)" for name,row in gates.items()]
    text += ["", "Engine/game behaviour coverage: NOT MEASURABLE YET; no whole-game percentage.",
             "RAC1 iPhone launch/menu/playability: NOT VALIDATED. Existing ReRAC milestones are separate.",
             "Real behaviour advanced: " + "; ".join(state.get("real_behaviour_advanced", [])),
             "Changes: " + "; ".join(state.get("changes", [])),
             "", "Tests/evidence:"]
    text += [f"- Scoped coverage: {json.dumps(metric)}" for metric in scoped]
    if "arm64_compilation" in state:
        text.append("- Generated ARM64 compile audit (not a game link): " + json.dumps(state["arm64_compilation"]))
    if "generated_source_compatibility" in state:
        text.append("- Mixed-optimization source compatibility only: " + json.dumps(state["generated_source_compatibility"]))
    if "generated_partial_link" in state:
        text.append("- Partial generated link (MH_OBJECT, NOT an executable): " + json.dumps(state["generated_partial_link"]))
    text += [f"- {json.dumps(row)}" for row in state["tests"]]
    text += ["", f"First blocker: {args.blocker}", f"Next safe action: {args.next_action}",
             "", "Classification: " + ", ".join(state["classification"]),
             "Shared Core: read/review only; no adoption. Other engines: unchanged.",
             "No quota exhaustion was reported. This file cannot detect account quota.",
             "Resume: read this checkpoint and JSON, verify source/receipt hashes, then perform the next safe action. Do not rerun completed codegen."]
    part = ROOT / "RAC1_IOS_PROGRESS.md.part"
    part.write_text("\n".join(text) + "\n")
    part.replace(ROOT / "RAC1_IOS_PROGRESS.md")
    print(f"CHECKPOINT={args.stop_reason} ELAPSED_SECONDS={elapsed} IPA_READINESS={delivery_score(gates)}/100")


if __name__ == "__main__": main()
